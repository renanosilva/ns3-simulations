/*
 * Copyright 2007 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "no-rollback-1.h"
#include "ns3/server-node-app.h"
#include "ns3/simulator.h"
#include "ns3/node-depleted-exception.h"
#include "ns3/node-asleep-exception.h"
#include "ns3/log-utils.h"
#include "ns3/application-type.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EARPWithoutRollback");

NS_OBJECT_ENSURE_REGISTERED(EARPWithoutRollback);
NS_OBJECT_ENSURE_REGISTERED(EventEARPWR);

TypeId
EARPWithoutRollback::GetTypeId()
{
    NS_LOG_FUNCTION(EARPWithoutRollback::GetTypeId());

    static TypeId tid =
        TypeId("ns3::EARPWithoutRollback")
            .SetParent<CheckpointStrategy>()
            .SetGroupName("Checkpoint")
            .AddConstructor<EARPWithoutRollback>()
            .AddAttribute("interval",
                          "Intervalo de tempo no qual serão criados checkpoints.",
                          TimeValue(Seconds(5.0)), //valor inicial
                          MakeTimeAccessor(&EARPWithoutRollback::interval),
                          MakeTimeChecker());
    
    return tid;
}

EARPWithoutRollback::EARPWithoutRollback(Time timeInterval, int nodesQuantity, Ptr<CheckpointApp> application){
    NS_LOG_FUNCTION(this);

    interval = timeInterval;
    rec_i = -1;
    eventCount = -1;
    app = application;
    creationScheduling = EventId();
    checkpointHelper = Create<CheckpointHelper>(application->getNodeName());
    adjacentNodes.clear();
    aliveNodes.clear();
    events.clear();
    csn = -1;
    totalNodesQuantity = nodesQuantity;
    rollbackInProgress = false;
    rollbackPhase2InProgress = false;
    newAppMsgsBuffer.clear();
    replayMsgsMap.clear();
}

EARPWithoutRollback::EARPWithoutRollback(){
    NS_LOG_FUNCTION(this);
    rec_i = -1;
    eventCount = -1;
    adjacentNodes.clear();
    events.clear();
    csn = -1;
    aliveNodes.clear();
    rollbackInProgress = false;
    rollbackPhase2InProgress = false;
    newAppMsgsBuffer.clear();
    replayMsgsMap.clear();
}

EARPWithoutRollback::~EARPWithoutRollback()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("\nESTRATÉGIA DE CHECKPOINT DESTRUÍDA\n");
}

void EARPWithoutRollback::DisposeReferences(){
    stopCheckpointing();
    
    adjacentNodes.clear();
    aliveNodes.clear();
    events.clear();
    newAppMsgsBuffer.clear();
    replayMsgsMap.clear();
    nodesMaxEventsMap.clear();
    expectedResentMsgsQuantityMap.clear();
}

void EARPWithoutRollback::startCheckpointing() {
    NS_LOG_FUNCTION(this);

    //Agendando a criação do checkpoint para logo após o início da aplicação
    // Time delay = Time("0.000000000000000000000001s");
    // creationScheduling = Simulator::Schedule(delay, &EARPWithoutRollback::writeCheckpoint, this);
    
    scheduleNextCheckpoint();
}

void EARPWithoutRollback::stopCheckpointing() {
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(creationScheduling);
}

void EARPWithoutRollback::writeCheckpoint() {
    NS_LOG_FUNCTION(this);

    if (!app->mayCheckpoint() || events.empty() || rollbackInProgress){
        scheduleNextCheckpoint();
        return;
    }

    try {
        json j = events;
        csn = checkpointHelper->getLastCheckpointId();
        checkpointHelper->writeCheckpoint(j, ++csn);

        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", CHECKPOINT " << csn << " CRIADO POR " 
            << checkpointHelper->getCheckpointBasename() << ".");

        events.clear();
        
        app->decreaseCheckpointEnergy();

        scheduleNextCheckpoint();

    } catch (NodeAsleepException& e) {
        //Operações interrompidas... Nó irá entrar em modo sleep. Nada mais a fazer.
    } catch (NodeDepletedException& e) {
        //Operações interrompidas... Nó irá entrar em modo depleted. Nada mais a fazer.
    } 
}

Time EARPWithoutRollback::getDelayToNextCheckpoint(){
    NS_LOG_FUNCTION(this);
    
    return interval;
}

void EARPWithoutRollback::scheduleNextCheckpoint(){
    NS_LOG_FUNCTION(this);

    //Agendando próximo checkpoint
    Time delay = getDelayToNextCheckpoint();

    //Será agendado com um delay calculado para garantir o intervalo de tempo predefinido
    creationScheduling = Simulator::Schedule(delay,
                                &EARPWithoutRollback::writeCheckpoint,
                                Ptr<EARPWithoutRollback>(this));
}

void EARPWithoutRollback::broadcastCheckAlive(){

    NS_LOG_FUNCTION(this);

    //Verificando se os nós adjacentes estão online através do envio de mensagens
    //Após os envios, fica-se aguardando as respostas dos nós

    if (adjacentNodes.size() > 0){

        for (Ptr<NodeInfoEARPWR> i : adjacentNodes){
            app->send(CHECK_ALIVE_COMMAND, 0, i->GetAddress());
        }

    }

}

void EARPWithoutRollback::rollbackToLastCheckpoint() {
    NS_LOG_FUNCTION(this);   
    
    try {

        rollbackInProgress = true;
        rollbackPhase2InProgress = false;
        aliveNodes.clear();

        rollback(getLastCheckpointId());
        broadcastCheckAlive();

    } catch (NodeAsleepException& e) {
        NS_LOG_LOGIC("Tarefa incompleta por estar em modo SLEEP.");
        return;
    } catch (NodeDepletedException& e) {
        NS_LOG_LOGIC("Tarefa incompleta por estar em modo DEPLETED.");
        return;
    }
}

bool EARPWithoutRollback::rollback(int cpId) {
    NS_LOG_FUNCTION(this);

    checkpointId = cpId;

    app->beforeRollback();

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << app->getNodeName() 
                                    << " iniciando procedimento de ROLLBACK para o checkpoint " << checkpointId << ".");
    
    json j;

    try {

        //lendo checkpoint
        j = checkpointHelper->readCheckpoint(checkpointId);

    } catch (const json::parse_error& e){
        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << app->getNodeName() 
                                    << " FALHOU ao tentar realizar ROLLBACK. Checkpoint "
                                    << checkpointId << " inválido.");
        return false;
    }
    
    //Recupera os eventos presentes no checkpoint
    events = j.get<vector<Ptr<EventEARPWR>>>();
    
    //Recupera o estado da aplicação presente no último evento do checkpoint
    app->from_json(getActiveEvent()->GetNodeState());
    
    /* A numeração de sequência de checkpoint é atualizada para que não haja
    sobrescrita de checkpoints */
    csn = checkpointHelper->getLastCheckpointId();

    //O checkpoint ativo é o último evento do checkpoint para o qual foi feito rollback
    rec_i = getActiveEvent()->GetJ();
    
    app->afterRollback();

    /* Fazendo o replay das mensagens presentes no evento carregado.
    Isso é necessário, pois o estado é referente ao momento imediatamente 
    anterior ao evento e o protocolo diz que, quando o rollback é feito,
    o sistema deve ficar no estado posterior ao processamento das
    mensagens do evento. */  
    
    replayEvent(getActiveEvent());
    
    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << app->getNodeName() 
            << " concluiu o procedimento de rollback.");

    app->decreaseRollbackEnergy();

    NS_LOG_INFO("\nDepois do rollback...");
    app->printNodeData();

    return true;
}

// void EARPWithoutRollback::notifyNodesAboutRollback(){
//     NS_LOG_FUNCTION(this);

//     // Enviando notificação para os nós com os quais houve comunicação
//     if (adjacentNodes.size() > 0){

//         for (NodeInfoEARPWR i : adjacentNodes){
            
//             /* Enviando solicitação de rollback para o destino. Se for apenas uma solicitação inicial de rollback,
//             (aviso de falha), não é enviado o valor de SENTi->j, e sim um valor simbólico de -1. */
            
//             Ptr<MessageData> md = app->send(REQUEST_TO_START_ROLLBACK_COMMAND, 
//                 (rollbackIteration == -1 ? -1 : i.GetMessagesSent()), 
//                 // i.GetMessagesSent(), 
//                 i.GetAddress());

//         }
//     }
// }

bool EARPWithoutRollback::interceptRead(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this << md);

    try {
        if (!app->isAlive()
            || ((md->GetCommand() == REQUEST_VALUE 
                || md->GetCommand() == RESPONSE_VALUE 
                || md->GetCommand() == ACKNOWLEDGEMENT_COMMAND) && rollbackInProgress)){
            
            return true;
        }

        if (md->GetCommand() == REQUEST_VALUE){
            return interceptRequestValueReceive(md);
        }
        
        if (md->GetCommand() == RESPONSE_VALUE){
            return interceptResponseValueReceive(md);
        }

        if (md->GetCommand() == ACKNOWLEDGEMENT_COMMAND){
            return interceptAckReceive(md);
        }

    } catch (NodeAsleepException& e) {
        NS_LOG_LOGIC("Tarefa incompleta por estar em modo SLEEP.");
        return true;
    } catch (NodeDepletedException& e) {
        NS_LOG_LOGIC("Tarefa incompleta por estar em modo DEPLETED.");
        return true;
    }
    
    return false;
}

bool EARPWithoutRollback::interceptRequestValueReceive(Ptr<MessageData> md, bool replay){
    NS_LOG_FUNCTION(this);

    //O recebimento de uma mensagem de request gera um novo evento
    Ptr<EventEARPWR> e = Create<EventEARPWR>();
    e->SetNodeState(app->to_json()); //O estado do nó a ser gravado é o de antes do processamento da mensagem
    e->SetJ(++eventCount);
    e->SetMessage(md);
    e->SetReceived(true);
    e->SetAcked(replay ? true : false);
    e->SetRemoteEventIndex(replay ? md->GetFirstPiggyBackedValue() : -1);
    
    events.push_back(e);
    processMessageReceived(md);

    return false;
}

bool EARPWithoutRollback::interceptResponseValueReceive(Ptr<MessageData> md, bool replay){
    NS_LOG_FUNCTION(this);
    
    //O recebimento de uma mensagem de response gera um novo evento (sem novos envios, ou seja, com mSent vazio)
    Ptr<EventEARPWR> e = Create<EventEARPWR>();
    e->SetNodeState(app->to_json()); //O estado do nó a ser gravado é o de antes do processamento da mensagem
    e->SetJ(++eventCount);
    e->SetMessage(md);
    e->SetReceived(true);
    e->SetAcked(replay ? true : false);
    e->SetRemoteEventIndex(md->GetFirstPiggyBackedValue()); //o emissor envia seu índice de evento

    events.push_back(e);
    processMessageReceived(md);

    return false;
}

bool EARPWithoutRollback::interceptAckReceive(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);
    
    //O recebimento de uma mensagem de ack demanda o registro local de recebimento da mensagem pelo emissor
    //Buscando a mensagem mais antiga que este nó enviou ao emissor do ack que ainda esteja sem confirmação
    Ptr<EventEARPWR> e = getNewestEventWithoutAck(md->GetFrom());

    //Atualizando evento para incluir a confirmação de recebimento
    e->SetAcked(true);
    e->SetRemoteEventIndex(md->GetData()); //j

    processMessageReceived(md);

    return false;
}

void EARPWithoutRollback::afterMessageReceive(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);

    if (md->GetCommand() == REQUEST_VALUE || md->GetCommand() == RESPONSE_VALUE){
        //Enviando confirmação de recebimento de mensagem (ack)
        //Junto ao ack, é enviado o índice referente ao evento de recebimento da mensagem original
        app->send(ACKNOWLEDGEMENT_COMMAND, getActiveEvent()->GetJ(), md->GetFrom());
        return;
    }

    if (md->GetCommand() == CHECK_ALIVE_COMMAND){
        afterCheckAliveReceive(md);
        return;
    }

    if (md->GetCommand() == I_AM_ALIVE_COMMAND){
        afterIAmAliveReceive(md);
        return;
    }

    if (md->GetCommand() == REQUEST_RESEND){
        afterRequestResendReceive(md);
        return;
    }

    //Se o comando for exatamente igual a RESEND_RESPONSE 
    if (md->GetCommand() == RESEND_RESPONSE){
        //Trata-se de uma mensagem para informar a maior numeração de evento deste nó que o emissor conhece
        afterResendResponseReceive(md);
        return;
    }

    //Se o comando contém o termo RESEND_RESPONSE
    if (md->GetCommand().find(RESEND_RESPONSE) != string::npos){
        //Trata-se do reenvio de uma mensagem
        afterResendMessageReceive(md);
        return;
    }

}

void EARPWithoutRollback::afterResendResponseReceive(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);
    
    //Atualizando mapa com informações sobre eventos
    nodesMaxEventsMap[md->GetFrom()] = md->GetData();
    expectedResentMsgsQuantityMap[md->GetFrom()] = md->GetFirstPiggyBackedValue();

    if (md->GetFirstPiggyBackedValue() == 0){
        //Não há mensagens a reenviar relativas ao nó emissor

        if (checkIfAllMsgsWereResent()){
            reprocessMessagesAndConcludeRollback();
        }
    }
}

void EARPWithoutRollback::afterResendMessageReceive(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);
    
    int eventIndex = md->GetFirstPiggyBackedValue();

    //Adicionando mensagem à fila de processamento que será executada quando todas as mensagens forem recebidas
    replayMsgsMap[eventIndex] = md;

    if (checkIfAllMsgsWereResent()){
        reprocessMessagesAndConcludeRollback();
    }
}

void EARPWithoutRollback::reprocessMessagesAndConcludeRollback(){
    //Apagando da memória volátil os eventos que foram carregados do checkpoint
    //Irão ficar apenas as mensagens que foram reenviadas
    events.clear();

    //Todas as mensagens já foram reenviadas, portanto, elas devem ser reexecutadas
    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S)
                    << ", " << app->getNodeName() 
                    << " recebeu todas as mensagens que deveriam ser reenviadas. Iniciando reprocessamento...");
    
    for (auto& par : replayMsgsMap){
        Ptr<MessageData> msgReceived = par.second;
        
        if (msgReceived->GetCommand().find(REQUEST_VALUE) != string::npos){
            reprocessRequestMessage(msgReceived);
        }
        
        if (msgReceived->GetCommand().find(RESPONSE_VALUE) != string::npos){
            /* Como foi reenviada uma mensagem de resposta, é necessário
            recriar a mensagem de request que foi enviada previamente à
            resposta por este nó. */
            Ptr<MessageData> md = Create<MessageData>();
            md->SetCommand(REPLAY_REQUEST_VALUE);
            md->SetData(0);
            md->SetFrom(msgReceived->GetTo());
            md->SetPiggyBackedInfo(msgReceived->GetPiggyBackedInfo());
            md->SetSequenceNumber(msgReceived->GetSequenceNumber());
            md->SetSize(msgReceived->GetSize());
            md->SetTo(msgReceived->GetFrom());
            md->SetUid(msgReceived->GetUid());
            
            interceptSend(md);
            getActiveEvent()->SetAcked(true);
            getActiveEvent()->SetRemoteEventIndex(msgReceived->GetFirstPiggyBackedValue());

            reprocessResponseMessage(msgReceived);
        }
    }

    //Rollback concluído
    rollbackInProgress = false;
    rollbackPhase2InProgress = false;
    aliveNodes.clear();
    nodesMaxEventsMap.clear();
    expectedResentMsgsQuantityMap.clear();
    replayMsgsMap.clear();

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S)
                    << ", " << app->getNodeName() 
                    << " concluiu o reprocessamento das mensagens. ROLLBACK CONCLUÍDO.\n");
    
    NS_LOG_INFO("Após o reprocessamento...");
    app->printNodeData();
}

void EARPWithoutRollback::reprocessRequestMessage(Ptr<MessageData> md){
    //Setando o comando para o valor padrão para que fique registrado dessa forma no checkpoint
    md->SetCommand(REPLAY_REQUEST_VALUE);
    
    //Processando o recebimento da mensagem
    interceptRequestValueReceive(md, true);

    app->replayReceive(md, true, false);
}

void EARPWithoutRollback::reprocessResponseMessage(Ptr<MessageData> md){
    //Setando o comando para o valor padrão para que fique registrado dessa forma no checkpoint
    md->SetCommand(REPLAY_RESPONSE_VALUE);

    //Processando o recebimento da mensagem
    interceptResponseValueReceive(md, true);

    app->replayReceive(md, true, false);
}

bool EARPWithoutRollback::checkIfAllMsgsWereResent(){
    NS_LOG_FUNCTION(this);
    
    if (nodesMaxEventsMap.size() != adjacentNodes.size()){
        return false;
    }

    map<Address, bool> nodesConcluded; //Mapa que indica quais nós concluíram seus reenvios
    map<Address, int> msgsResentQuantityPerNode; //Mapa que indica quantas mensagens foram reenviadas por cada nó

    //Verificando se a quantidade de mensagens recebidas é igual à quantidade de mensagens esperadas de cada nó
    if (!replayMsgsMap.empty()){
        for (const auto& par : replayMsgsMap){
            Ptr<MessageData> m = par.second;
            
            //Contabilizando que o nó emissor enviou a mensagem
            int q = msgsResentQuantityPerNode[m->GetFrom()];
            msgsResentQuantityPerNode[m->GetFrom()] = ++q;

            //Se atingiu a quantidade de mensagens esperadas desse nó
            if (q == expectedResentMsgsQuantityMap[m->GetFrom()]){
                nodesConcluded[m->GetFrom()] = true;
            }
        } 
    }

    if (nodesConcluded.size() == adjacentNodes.size())
        return true;

    //Verificando se algum nó não tem mensagens a reenviar
    bool allConcluded = true;

    if (!adjacentNodes.empty()){
        for (Ptr<NodeInfoEARPWR> n : adjacentNodes){
            
            //Se o nó adjacente não está na lista de nós que concluíram e possui mensagens a reenviar
            if (nodesConcluded.find(n->GetAddress()) == nodesConcluded.end() 
                && expectedResentMsgsQuantityMap.find(n->GetAddress()) != expectedResentMsgsQuantityMap.end()
                && expectedResentMsgsQuantityMap[n->GetAddress()] > 0) {
                
                    allConcluded = false;
            }
        }
    }

    return allConcluded;

    // bool messagesToResend = false;

    // if (!msgsResentQuantityPerNode.empty()){
    //     for (const auto& par : msgsResentQuantityPerNode){
    //         int qtt = par.second;

    //         if (qtt > 0){
    //             messagesToResend = true;
    //             break;
    //         }
    //     }
    // }

    // if (!messagesToResend)
    //     return true;
    
    // return false;
}

void EARPWithoutRollback::afterRequestResendReceive(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);

    vector<Ptr<EventEARPWR>> eventsToAnalyze;
    vector<Ptr<MessageData>> msgsToResend;
    int remoteIndex = md->GetData();

    /* Procurando a sequência de mensagens que este nó enviou ao nó defeituoso q e que foram 
    recebidas e processadas por q depois do evento j de q.
    j é representado neste algoritmo pelo atributo remoteEventIndex. */

    //Procurando nos checkpoints anteriores de forma crescente
    int lastCheckpointId = getLastCheckpointId();
    int checkpointId = 0;

    while (checkpointId <= lastCheckpointId){
        //lendo checkpoint
        json j = checkpointHelper->readCheckpoint(checkpointId);

        //Recupera os eventos presentes no checkpoint
        eventsToAnalyze = j.get<vector<Ptr<EventEARPWR>>>();

        vector<Ptr<MessageData>> newMsgs = searchForMsgsToResend(eventsToAnalyze, md->GetFrom(), remoteIndex);

        if (!newMsgs.empty()){
            //Inserindo no final da coleção para que fique em ordem cronológica
            msgsToResend.insert(msgsToResend.end(), newMsgs.begin(), newMsgs.end());
        }

        checkpointId++;
    }

    // Procurando por eventos na memória volátil
    vector<Ptr<MessageData>> newMsgs = searchForMsgsToResend(events, md->GetFrom(), remoteIndex);
    msgsToResend.insert(msgsToResend.end(), newMsgs.begin(), newMsgs.end());

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S)
                        << ", " << app->getNodeName() 
                        << " reenviando " << msgsToResend.size() << " mensagens para "
                        << InetSocketAddress::ConvertFrom(md->GetFrom()).GetIpv4() << ".");
    
    //Calculando qual a maior numeração de evento do nó que falhou que este nó conhece
    int max_p_q = -1;
    
    if (!msgsToResend.empty()){
        for (Ptr<MessageData> m : msgsToResend){
            int v = m->GetFirstPiggyBackedValue();        
            max_p_q = max(max_p_q, v);
        }
    }

    //Enviando informações sobre o reenvio
    string piggyBackedInfo = "msgsToResendSize " + to_string(msgsToResend.size());
    app->send(RESEND_RESPONSE, max_p_q, md->GetFrom(), false, piggyBackedInfo);

    //Reenviando mensagens anteriormente enviadas
    if (msgsToResend.size() > 0){
        for (Ptr<MessageData> m : msgsToResend){
            app->resend(m);
        }
    }
}

vector<Ptr<MessageData>> EARPWithoutRollback::searchForMsgsToResend(vector<Ptr<EventEARPWR>> events, Address a, int j){
    NS_LOG_FUNCTION(this);
    
    vector<Ptr<MessageData>> msgsToResend;
    
    if (!events.empty()){
        for (Ptr<EventEARPWR> e : events){
            
            //Somente interessam as mensagens que foram enviadas após o último evento registrado no nó defeituoso
            if (e->GetRemoteEventIndex() > j && !e->GetMSent().empty()){
                //Verificando se o evento em questão possui mensagem enviada ao nó defeituoso
                for (Ptr<MessageData> m : e->GetMSent()){
                    if (m->GetTo() == a){
                        msgsToResend.push_back(m);
                    }
                }
            }

        }
    }

    return msgsToResend;
}

void EARPWithoutRollback::afterCheckAliveReceive(Ptr<MessageData> md) {
    //Respondendo que este nó está vivo
    app->send(I_AM_ALIVE_COMMAND, 0, md->GetFrom());

    //Se este nó está na fase 1 do rollback (aguardando confirmações de disponibilidade de outros nós)
    if (rollbackInProgress && !rollbackPhase2InProgress) {
        
        //Caso ele não esteja no vetor de nós vivos
        if (find(aliveNodes.begin(), aliveNodes.end(), md->GetFrom()) == aliveNodes.end()){
            //A mensagem em questão serve como confirmação de que o nó está vivo
            afterIAmAliveReceive(md);
        }
        
    }
}

void EARPWithoutRollback::afterIAmAliveReceive(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);
    
    aliveNodes.push_back(md->GetFrom());

    //Se já havia iniciado a segunda fase de recuperação
    if (rollbackPhase2InProgress){
        //Então a mensagem recebida é de algum nó que estava vivo mas que demorou um pouco mais a chegar
        //Nesse caso, executa-se a segunda parte do rollback somente com esse nó que ficou faltando

        requestResend(md->GetFrom());

    } else {

        //A segunda fase do rollback ainda não começou. Verificando se pode começar.
        //Se houver no máximo um nó adjacente com falha
        if (aliveNodes.size() >= adjacentNodes.size() - 1){

            //Então pode iniciar a segunda fase
            rollbackPhase2InProgress = true;

            NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S)
                        << ", " << app->getNodeName() 
                        << " obteve as confirmações necessárias para prosseguir com o rollback.");

            requestResendToAliveNodes();
        }

    }
}

void EARPWithoutRollback::requestResendToAliveNodes(){
    NS_LOG_FUNCTION(this);
    
    if (!aliveNodes.empty()){
        for (Address a : aliveNodes){
            requestResend(a);
        }
    }
}

void EARPWithoutRollback::requestResend(Address to){
    NS_LOG_FUNCTION(this);

    app->send(REQUEST_RESEND, rec_i, to);
}

bool EARPWithoutRollback::interceptSend(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);

    if (!app->isAlive()
        || ((md->GetCommand() == REQUEST_VALUE 
            || md->GetCommand() == RESPONSE_VALUE 
            || md->GetCommand() == ACKNOWLEDGEMENT_COMMAND) && rollbackInProgress)){
            
            return true;
    }

    if (md->GetCommand() == REQUEST_VALUE || md->GetCommand() == REPLAY_REQUEST_VALUE){
        
        //O envio de uma mensagem de request gera um novo evento
        Ptr<EventEARPWR> e = Create<EventEARPWR>();
        e->SetNodeState(app->to_json()); //O estado do nó a ser gravado é o de antes do processamento da mensagem
        e->SetJ(++eventCount);
        e->SetMSent(vector<Ptr<MessageData>>{md});
        e->SetAcked(false);
        e->SetReceived(false);
        e->SetRemoteEventIndex(-1);

        events.push_back(e);
        processMessageSent(md);
    }

    if (md->GetCommand() == RESPONSE_VALUE || md->GetCommand() == REPLAY_RESPONSE_VALUE){
        
        /* Uma mensagem do tipo response é enviada em decorrência do recebimento de uma
        mensagem anterior. Nesse caso, deve-se atualizar o último evento registrado. */
        Ptr<EventEARPWR> e = getActiveEvent();
        e->SetMSent(vector<Ptr<MessageData>>{md});
        
        processMessageSent(md);

    }

    if (md->GetCommand() == ACKNOWLEDGEMENT_COMMAND){
        processMessageSent(md);
    }

    return false;
}

void EARPWithoutRollback::processMessageReceived(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);
    
    addToAdjacentNodes(md->GetFrom());
    
    //Atualizando informações de quantidade de mensagens recebidas
    Ptr<NodeInfoEARPWR> result = findAdjacentNodeByAddress(md->GetFrom());
    int mReceived = result->GetMessagesReceived();
    result->SetMessagesReceived(mReceived + 1);
}

void EARPWithoutRollback::processMessageSent(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);
    
    addToAdjacentNodes(md->GetTo());
        
    //Atualizando informações de quantidade de mensagens enviadas
    Ptr<NodeInfoEARPWR> result = findAdjacentNodeByAddress(md->GetTo());
    int mSent = result->GetMessagesSent();
    result->SetMessagesSent(mSent + 1);

    md->SetPiggyBackedInfo("activeEventIndex " + to_string(getActiveEvent()->GetJ()));
}

Ptr<EventEARPWR> EARPWithoutRollback::getNewestEventWithoutAck(Address addr) {
    NS_LOG_FUNCTION(this);
    
    if (!events.empty()){
        for (auto i = events.size() - 1; i >= 0; i--){
            Ptr<EventEARPWR> e = events.at(i);
            
            if (!e->IsAcked() && !e->GetMSent().empty()){
                for (Ptr<MessageData> md : e->GetMSent()){
                    if (md->GetTo() == addr){
                        return e;
                    }
                }
            }

        }
    }

    NS_ABORT_MSG("Não foi encontrado um evento sem confirmação de ack.");
}

void EARPWithoutRollback::replayEvent(Ptr<EventEARPWR> e){
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) 
                << ", " << app->getNodeName() 
                << " realizando replay das mensagens presentes no evento " 
                << e->GetJ() << "...");

    Ptr<MessageData> msgReceived = e->GetMessage();

    //Se houve alguma mensagem recebida no evento
    if (msgReceived != nullptr && msgReceived->GetCommand() != ""){
        
        //Faz o processamento do recebimento
        
        addToAdjacentNodes(msgReceived->GetFrom());

        //Atualizando informações de quantidade de mensagens recebidas
        Ptr<NodeInfoEARPWR> result = findAdjacentNodeByAddress(msgReceived->GetFrom());
        int quantReceived = result->GetMessagesReceived();
        result->SetMessagesReceived(quantReceived + 1);
            
        app->replayReceive(msgReceived, false, false);
    }

    //Se houve alguma mensagem enviada no evento
    if (!e->GetMSent().empty()){
        
        for (Ptr<MessageData> md : e->GetMSent()){
                
            addToAdjacentNodes(md->GetTo());
        
            //Atualizando informações de quantidade de mensagens enviadas
            Ptr<NodeInfoEARPWR> result = findAdjacentNodeByAddress(md->GetTo());
            int mSent = result->GetMessagesSent();
            result->SetMessagesSent(mSent + 1);
            
            app->send(md->GetCommand(), md->GetData(), md->GetTo(), true);
            
        }
    }

    ++eventCount;
}

void EARPWithoutRollback::addToAdjacentNodes(Address a){
    NS_LOG_FUNCTION(this);
    
    Ptr<NodeInfoEARPWR> result = findAdjacentNodeByAddress(a);

    if (result == nullptr) {
        
        Ptr<NodeInfoEARPWR> i = CreateObject<NodeInfoEARPWR>(a);

        //Endereço não foi encontrado
        //Adiciona novo elemento ao vetor
        adjacentNodes.push_back(i);
    
    }
}

Ptr<NodeInfoEARPWR> EARPWithoutRollback::findAdjacentNodeByAddress(vector<Ptr<NodeInfoEARPWR>> v, Address addr){
    NS_LOG_FUNCTION(this);
    
    auto it = find_if(v.begin(), v.end(), [&addr](const Ptr<NodeInfoEARPWR> n) {
        return n->GetAddress() == addr;
    });

    if (it != v.end()) {
        return it->operator->(); // Retorna ponteiro para o NodeInfoEARPWR encontrado
    }

    return nullptr; // Não encontrado
}

Ptr<NodeInfoEARPWR> EARPWithoutRollback::findAdjacentNodeByAddress(Address addr) {
    NS_LOG_FUNCTION(this);
    return findAdjacentNodeByAddress(adjacentNodes, addr);
}

Ptr<EventEARPWR> EARPWithoutRollback::getActiveEvent(){
    NS_LOG_FUNCTION(this);

    if (events.size() == 0)
        return nullptr;
    
    return events.at(events.size() - 1);
}

void EARPWithoutRollback::printData() {
    NS_LOG_INFO("rec_i = " << rec_i 
                << ", csn = " << csn
                << ", adjacentNodes.size() = " << adjacentNodes.size() 
                << ", events.size() = " << events.size()
                << ", eventCount = " << eventCount
                << ", checkpointId = " << checkpointId
                << "\n");
}

void to_json(json& j, const Ptr<NodeInfoEARPWR>& obj) {
    NS_LOG_FUNCTION("to_json(json& j, const Ptr<NodeInfoEARPWR>& obj)");
    
    j = json{
        {"address", obj->address},
        {"messagesSent", obj->messagesSent},
        {"messagesReceived", obj->messagesReceived}
    };
}

void from_json(const json& j, Ptr<NodeInfoEARPWR>& obj) {
    NS_LOG_FUNCTION("from_json(const json& j, Ptr<NodeInfoEARPWR>& obj)");
    
    obj = CreateObject<NodeInfoEARPWR>();
    j.at("address").get_to(obj->address);
    j.at("messagesSent").get_to(obj->messagesSent);
    j.at("messagesReceived").get_to(obj->messagesReceived);
}

// void to_json(json& j, const vector<Ptr<MessageData>>& vec){
//     j = json::array();

//     for (const auto& ptr : vec){
//         if (ptr != nullptr){
//             json item;
//             to_json(item, *ptr); // reuso do to_json(MessageData&)
//             j.push_back(item);
//         }
//     }
// }

// void from_json(const json& j, vector<Ptr<MessageData>>& vec){
//     vec.clear();

//     for (const auto& item : j){
//         // desserializa para objeto temporário
//         MessageData tmp;
//         from_json(item, tmp); // reuso do seu from_json(MessageData&)

//         // converte temporário em Ptr
//         Ptr<MessageData> ptr = CreateObject<MessageData>();
//         *ptr = tmp; // atribuição de valores
//         vec.push_back(ptr);
//     }
// }

// void to_json(json& j, const vector<Ptr<NodeInfoEARPWR>>& vec){
//     j = json::array();

//     for (const auto& ptr : vec){
//         if (ptr != nullptr){
//             json item;
//             to_json(item, *ptr); // reuso do to_json(NodeInfoEARPWR&)
//             j.push_back(item);
//         }
//     }
// }

// void from_json(const json& j, std::vector<Ptr<NodeInfoEARPWR>>& vec){
//     vec.clear();

//     for (const auto& item : j){
//         // desserializa para objeto temporário
//         NodeInfoEARPWR tmp;
//         from_json(item, tmp); // reuso do seu from_json(NodeInfoEARPWR&)

//         // converte temporário em Ptr
//         Ptr<NodeInfoEARPWR> ptr = CreateObject<NodeInfoEARPWR>();
//         *ptr = tmp; // atribuição de valores
//         vec.push_back(ptr);
//     }
// }

void to_json(json& j, const Ptr<EventEARPWR>& obj) {
    NS_LOG_FUNCTION("to_json(json& j, const Ptr<EventEARPWR>& obj)");
    
    j = json {
        {"j", obj->j},
        {"nodeState", obj->nodeState},
        {"m", obj->m},
        {"mSent", obj->mSent},
        {"received", obj->received},
        {"acked", obj->acked},
        {"remoteEventIndex", obj->remoteEventIndex}
    };
}

void from_json(const json& j, Ptr<EventEARPWR>& obj) {
    NS_LOG_FUNCTION("from_json(const json& j, Ptr<EventEARPWR>& obj)");

    obj = CreateObject<EventEARPWR>();
    j.at("j").get_to(obj->j);
    j.at("nodeState").get_to(obj->nodeState);
    j.at("received").get_to(obj->received);
    j.at("acked").get_to(obj->acked);
    j.at("remoteEventIndex").get_to(obj->remoteEventIndex);
    j.at("m").get_to(obj->m);
    j.at("mSent").get_to(obj->mSent);
}

json EARPWithoutRollback::to_json() {
    NS_LOG_FUNCTION("EARPWithoutRollback::to_json");
    
    json j = CheckpointStrategy::to_json();
    j["adjacentNodes"] = adjacentNodes;
    j["eventCount"] = eventCount;
    j["csn"] = csn;
    j["totalNodesQuantity"] = totalNodesQuantity;
    
    return j;
}

void EARPWithoutRollback::from_json(const json& j) {
    NS_LOG_FUNCTION("EARPWithoutRollback::from_json");
    
    CheckpointStrategy::from_json(j);
    j.at("adjacentNodes").get_to(adjacentNodes);
    j.at("eventCount").get_to(eventCount);
    j.at("csn").get_to(csn);
    j.at("totalNodesQuantity").get_to(totalNodesQuantity);
}

} // Namespace ns3
