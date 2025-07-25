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

#include "earp-without-rollback-v2.h"
#include "ns3/server-node-app.h"
#include "ns3/simulator.h"
#include "ns3/node-depleted-exception.h"
#include "ns3/node-asleep-exception.h"
#include "ns3/log-utils.h"
#include "ns3/application-type.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EARPWithoutRollbackV2");

NS_OBJECT_ENSURE_REGISTERED(EARPWithoutRollbackV2);
NS_OBJECT_ENSURE_REGISTERED(EventEARPWRv2);

TypeId
EARPWithoutRollbackV2::GetTypeId()
{
    NS_LOG_FUNCTION(EARPWithoutRollbackV2::GetTypeId());

    static TypeId tid =
        TypeId("ns3::EARPWithoutRollbackV2")
            .SetParent<CheckpointStrategy>()
            .SetGroupName("Checkpoint")
            .AddConstructor<EARPWithoutRollbackV2>()
            .AddAttribute("interval",
                          "Intervalo de tempo no qual serão criados checkpoints.",
                          TimeValue(Seconds(5.0)), //valor inicial
                          MakeTimeAccessor(&EARPWithoutRollbackV2::interval),
                          MakeTimeChecker());
    
    return tid;
}

EARPWithoutRollbackV2::EARPWithoutRollbackV2(Time timeInterval, int nodesQuantity, Ptr<CheckpointApp> application){
    NS_LOG_FUNCTION(this);

    interval = timeInterval;
    rec_i = -1;
    eventCount = -1;
    app = application;
    creationScheduling = EventId();
    checkpointHelper = Create<CheckpointHelper>(application->getNodeName());
    adjacentNodes.clear();
    events.clear();
    senderIds.clear();
    outIds.clear();
    resentMsgsMap.clear();
    failedNodes.clear();
    csn = -1;
    totalNodesQuantity = nodesQuantity;
    rollbackInProgress = false;
}

EARPWithoutRollbackV2::EARPWithoutRollbackV2(){
    NS_LOG_FUNCTION(this);
    rec_i = -1;
    eventCount = -1;
    adjacentNodes.clear();
    events.clear();
    senderIds.clear();
    outIds.clear();
    resentMsgsMap.clear();
    failedNodes.clear();
    csn = -1;
    rollbackInProgress = false;
}

EARPWithoutRollbackV2::~EARPWithoutRollbackV2()
{
    NS_LOG_FUNCTION(this);
}

void EARPWithoutRollbackV2::DisposeReferences(){
    stopCheckpointing();
    
    adjacentNodes.clear();
    events.clear();
    senderIds.clear();
    outIds.clear();
    resentMsgsMap.clear();
    failedNodes.clear();
}

void EARPWithoutRollbackV2::startCheckpointing() {
    NS_LOG_FUNCTION(this);
    scheduleNextCheckpoint();
}

void EARPWithoutRollbackV2::stopCheckpointing() {
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(creationScheduling);
}

void EARPWithoutRollbackV2::writeCheckpoint() {
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

Time EARPWithoutRollbackV2::getDelayToNextCheckpoint(){
    NS_LOG_FUNCTION(this);
    
    return interval;
}

void EARPWithoutRollbackV2::scheduleNextCheckpoint(){
    NS_LOG_FUNCTION(this);

    //Agendando próximo checkpoint
    Time delay = getDelayToNextCheckpoint();

    //Será agendado com um delay calculado para garantir o intervalo de tempo predefinido
    creationScheduling = Simulator::Schedule(delay,
                                &EARPWithoutRollbackV2::writeCheckpoint,
                                Ptr<EARPWithoutRollbackV2>(this));
}

void EARPWithoutRollbackV2::rollbackToLastCheckpoint() {
    NS_LOG_FUNCTION(this);   
    
    try {
        //Iniciando rollback
        rollbackInProgress = true;
        rollback(getLastCheckpointId());

        //Iniciando restauração de um estado consistente. Lendo IDs de emissores de mensagens...
        senderIds = checkpointHelper->readLog(getSenderIdsLogName());
    
        //y é a quantidade de mensagens recebidas antes da falha por este nó que foram perdidas
        int y = senderIds.size() - GetTotalMsgsReceived();
        
        //Armazenando os y últimos IDs de emissores em outIds
        outIds.insert(outIds.end(), senderIds.end() - y, senderIds.end());
        
        requestResendToAdjacentNodes();

        //Reseta os eventos da memória volátil pois eles já estão registrados em checkpoint
        events.clear();

    } catch (NodeAsleepException& e) {
        NS_LOG_LOGIC("Tarefa incompleta por estar em modo SLEEP.");
        return;
    } catch (NodeDepletedException& e) {
        NS_LOG_LOGIC("Tarefa incompleta por estar em modo DEPLETED.");
        return;
    }
}

bool EARPWithoutRollbackV2::rollback(int cpId) {
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
    events = j.get<vector<Ptr<EventEARPWRv2>>>();
    
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

bool EARPWithoutRollbackV2::interceptRead(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this << md);

    try {
        if (!app->isAlive()
            || ((md->GetCommand() == REQUEST_VALUE 
                || md->GetCommand() == RESPONSE_VALUE) 
                && rollbackInProgress)){
            
            return true;
        }

        if (md->GetCommand() == REQUEST_VALUE){
            logSenderId(md);
            return interceptRequestValueReceive(md);
        }
        
        if (md->GetCommand() == RESPONSE_VALUE){
            logSenderId(md);
            return interceptResponseValueReceive(md);
        }

        if (md->GetCommand() == RESEND_RESPONSE + "-" + REQUEST_VALUE && !rollbackInProgress){
            /* Se recebeu uma mensagem de RESEND não estando em modo de rollback,
            é porque se trata de uma mensagem perdida que foi reenviada. Nesse
            caso, deve-se tratá-la como uma requisição normal. */
            md->SetCommand(REQUEST_VALUE);
            logSenderId(md);
            return interceptRequestValueReceive(md);
        }

        if (md->GetCommand() == RESEND_RESPONSE + "-" + RESPONSE_VALUE && !rollbackInProgress){
            /* Se recebeu uma mensagem de RESEND não estando em modo de rollback,
            é porque se trata de uma mensagem perdida que foi reenviada. Nesse
            caso, deve-se tratá-la como uma requisição normal. */
            
            //Recriando requisição original
            app->send(REQUEST_VALUE, 0, md->GetFrom(), true);

            //Processando o recebimento da mensagem
            md->SetCommand(RESPONSE_VALUE);
            logSenderId(md);
            interceptResponseValueReceive(md);

            //app->replayReceive(md, false, false);
            
            return false;
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

bool EARPWithoutRollbackV2::interceptRequestValueReceive(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);

    //O recebimento de uma mensagem de request gera um novo evento
    Ptr<EventEARPWRv2> e = Create<EventEARPWRv2>();
    e->SetNodeState(app->to_json()); //O estado do nó a ser gravado é o de antes do processamento da mensagem
    e->SetJ(++eventCount);
    e->SetMessage(md);
    
    events.push_back(e);
    updateNodeInfoAfterReceive(md);

    return false;
}

bool EARPWithoutRollbackV2::interceptResponseValueReceive(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);
    
    //O recebimento de uma mensagem de response gera um novo evento (sem novos envios, ou seja, com mSent vazio)
    Ptr<EventEARPWRv2> e = Create<EventEARPWRv2>();
    e->SetNodeState(app->to_json()); //O estado do nó a ser gravado é o de antes do processamento da mensagem
    e->SetJ(++eventCount);
    e->SetMessage(md);

    events.push_back(e);
    updateNodeInfoAfterReceive(md);

    return false;
}

void EARPWithoutRollbackV2::afterMessageReceive(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);

    if (md->GetCommand() == REQUEST_RESEND){
        afterRequestResendReceive(md);
        return;
    }

    //Se o comando contém o termo RESEND_RESPONSE
    if (md->GetCommand().find(RESEND_RESPONSE) != string::npos
        || md->GetCommand() == REPLAY_RESPONSE_VALUE){

        afterResendMessageReceive(md);
        return;
    }
}

void EARPWithoutRollbackV2::afterRequestResendReceive(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);

    //Foi iniciado um processo de recuperação de outro nó
    if (!rollbackInProgress){
        //Caso este nó também não esteja em recuperação, deve-se resetar o outIds
        outIds.clear();
    }

    int msgsReceivedByRemoteNode = md->GetData();
    Ptr<NodeInfoEARPWR> nodeInfo = findAdjacentNodeByAddress(md->GetFrom());

    //Registrando que o nó emissor falhou
    failedNodes[md->GetFrom()] = msgsReceivedByRemoteNode;

    if (nodeInfo->GetMessagesSent() > msgsReceivedByRemoteNode){

        /* Procurando evento tal que o número de mensagens enviadas seja igual ao de mensagens
        recebidas pelo nó remoto. */
        Ptr<EventEARPWRv2> e = searchForEvent(md->GetFrom(), msgsReceivedByRemoteNode);
        vector<Ptr<MessageData>> msgsToResend = searchForMsgsToResend(e, md->GetFrom());

        app->decreaseCheckpointEnergy();

        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S)
                        << ", " << app->getNodeName() 
                        << " reenviando " << msgsToResend.size() << " mensagens para "
                        << InetSocketAddress::ConvertFrom(md->GetFrom()).GetIpv4() << ".");

        //Reenviando mensagens anteriormente enviadas
        if (msgsToResend.size() > 0){
            for (Ptr<MessageData> m : msgsToResend){
                app->resend(m);
            }
        }
    
    } else if (nodeInfo->GetMessagesSent() == msgsReceivedByRemoteNode) {

        //Responde informando que não há mensagens a reenviar
        app->send(RESEND_RESPONSE, -1, md->GetFrom());

    } else {
        /* Neste caso, este nó também falhou e está aguardando reenvio de mensagens.
        Será necessário reenviar mensagens que não estão registradas no momento. */

        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S)
                    << ", " << app->getNodeName() 
                    << " identificou que não possui as mensagens solicitadas pois está em falha. Aguardando "
                    << "reenvio de outros nós...");

        //Registrando informações sobre o nó remoto para posterior reenvio das mensagens solicitadas
        failedNodes[md->GetFrom()] = msgsReceivedByRemoteNode;

        /* Como este nó está em falha e identificou que o nó remoto acabou de se recuperar de uma falha,
        este nó irá reenviar a requisição de reenvio de mensagens para o nó remoto, pois certamente ele
        não tinha recebido. */
        requestResend(findAdjacentNodeByAddress(md->GetFrom()));
    }
}

void EARPWithoutRollbackV2::afterResendMessageReceive(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);

    if (md->GetCommand() == RESEND_RESPONSE && md->GetData() == -1){
        //O nó em questão não tinha mensagens a reenviar
        
    } else {
        //Adicionando mensagem à fila de reprocessamento
        resentMsgsMap[md->GetFrom()].push_back(md);
    }
    
    bool done = false;

    while (!done && !outIds.empty()){

        Address a = outIds.at(0);
        vector<Ptr<MessageData>>& msgsQueue = resentMsgsMap[a];

        if (!msgsQueue.empty()){
            Ptr<MessageData> m = msgsQueue.at(0);
            outIds.erase(outIds.begin());
            msgsQueue.erase(resentMsgsMap[a].begin());

            reprocessMessage(m);

        } else {
            done = true;
        }
    }

    if (outIds.empty()){
        //Concluindo rollback
        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S)
                    << ", " << app->getNodeName() 
                    << " recebeu todas as mensagens que deveriam ser reenviadas. ROLLBACK CONCLUÍDO.");

        rollbackInProgress = false;
        resentMsgsMap.clear();
        failedNodes.clear();

        NS_LOG_INFO("\nApós o reprocessamento...");
        app->printNodeData();
    }
}

void EARPWithoutRollbackV2::reprocessMessage(Ptr<MessageData> md){
    if (md->GetCommand().find(REQUEST_VALUE) != string::npos){
        reprocessRequestMessage(md);
    } else if (md->GetCommand().find(RESPONSE_VALUE) != string::npos){
        reprocessResponseMessage(md);
    }
}

void EARPWithoutRollbackV2::reprocessRequestMessage(Ptr<MessageData> md){
    
    //Processando o recebimento da mensagem
    interceptRequestValueReceive(md);

    //Indica se deve enviar uma mensagem de resposta
    bool resendResponse = false;

    //Verifica se o emissor também falhou
    bool senderHasFailed = failedNodes.find(md->GetFrom()) != failedNodes.end();
    
    if (senderHasFailed){
        Ptr<NodeInfoEARPWR> info = findAdjacentNodeByAddress(md->GetFrom());
        int msgsSent = info->GetMessagesSent();
        int msgsReceived = failedNodes[md->GetFrom()];

        if (msgsSent > msgsReceived){
            resendResponse = true;
        }
    }

    app->replayReceive(md, true, resendResponse);
}

void EARPWithoutRollbackV2::reprocessResponseMessage(Ptr<MessageData> md){

    /* Como foi reenviada uma mensagem de resposta, é necessário
    recriar a mensagem de request que foi enviada previamente à
    resposta por este nó. Nesse caso, como a resposta foi reenviada
    para este nó, nunca vai ser necessário reenviar a requisição 
    que originou essa resposta. */

    //Forçando que o(s) envio(s) seja(m) interceptado(s) pelo protocolo
    rollbackInProgress = false; 
    
    //Recriando requisição original
    app->send(REQUEST_VALUE, 0, md->GetFrom(), true);

    //Processando o recebimento da resposta
    interceptResponseValueReceive(md);

    app->replayReceive(md, false, false);

    //Verificando se é necessário enviar uma nova requisição
    bool remoteNodeHasFailed = failedNodes.find(md->GetFrom()) != failedNodes.end();
    
    if (remoteNodeHasFailed){
        Ptr<NodeInfoEARPWR> remoteNodeInfo = findAdjacentNodeByAddress(md->GetFrom());
        int msgsSentToRemoteNode = remoteNodeInfo->GetMessagesSent();
        int msgsReceivedByRemoteNode = failedNodes[md->GetFrom()];
        bool msgsSentIsEqualToMsgsReceived = msgsSentToRemoteNode == msgsReceivedByRemoteNode;
        bool remoteNodeHasResentMsgsInQueue = !resentMsgsMap[md->GetFrom()].empty();
        bool remoteNodeIsNextInOutIds = outIds.at(0) == md->GetFrom();

        if (!remoteNodeHasResentMsgsInQueue && msgsSentIsEqualToMsgsReceived && remoteNodeIsNextInOutIds){
            app->send(RESEND_RESPONSE + "-" + REQUEST_VALUE, 0, md->GetFrom());
        }
    }

    rollbackInProgress = true;
}

vector<Ptr<MessageData>> EARPWithoutRollbackV2::searchForMsgsToResend(Ptr<EventEARPWRv2> e, Address to){
    NS_LOG_FUNCTION(this);
    
    vector<Ptr<MessageData>> msgsToResend;
    
    //Procurando inicialmente na memória volátil
    if (!events.empty()){
        Ptr<EventEARPWRv2> firstVolatileEvent = events.at(0);

        if (firstVolatileEvent->GetJ() <= e->GetJ()){
            //O evento se encontra na memória volátil. Obtendo mensagens...

            vector<Ptr<MessageData>> newMsgs = searchForMsgsToResend(events, e, to);
            msgsToResend.insert(msgsToResend.end(), newMsgs.begin(), newMsgs.end());
            return msgsToResend;
        }
    }

    //Procurando nos checkpoints anteriores de forma decrescente
    int cpId = checkpointHelper->getLastCheckpointId();

    while (cpId >= 0){
        
        //lendo checkpoint
        json j = checkpointHelper->readCheckpoint(cpId);

        //Recupera os eventos presentes no checkpoint
        vector<Ptr<EventEARPWRv2>> eventsToAnalyze = j.get<vector<Ptr<EventEARPWRv2>>>();
        Ptr<EventEARPWRv2> firstEvent = eventsToAnalyze.at(0);

        if (firstEvent->GetJ() <= e->GetJ()){
            /* O evento se encontra no checkpoint analisado. Deve-se verificar por mensagens
            a partir deste checkpoint até o último. */

            for (int i = cpId; i <= checkpointHelper->getLastCheckpointId(); i++){
                j = checkpointHelper->readCheckpoint(i);
                eventsToAnalyze = j.get<vector<Ptr<EventEARPWRv2>>>();
                vector<Ptr<MessageData>> newMsgs = searchForMsgsToResend(eventsToAnalyze, e, to);
                msgsToResend.insert(msgsToResend.end(), newMsgs.begin(), newMsgs.end());
            }

            //Incluindo agora os que se encontram na memória volátil.
            vector<Ptr<MessageData>> newMsgs = searchForMsgsToResend(events, e, to);
            msgsToResend.insert(msgsToResend.end(), newMsgs.begin(), newMsgs.end());

            return msgsToResend;
        }

        cpId--;
    }

    return msgsToResend;
}

vector<Ptr<MessageData>> EARPWithoutRollbackV2::searchForMsgsToResend(vector<Ptr<EventEARPWRv2>> events, Ptr<EventEARPWRv2> e, Address to){
    NS_LOG_FUNCTION(this);
    
    vector<Ptr<MessageData>> msgsToResend;
    
    if (!events.empty()){
        for (Ptr<EventEARPWRv2> evt : events){
            
            //Somente interessam as mensagens que foram enviadas após o evento "e" para o nó "to"
            if (evt->GetJ() >= e->GetJ() && !evt->GetMSent().empty()){
                //Verificando se o evento em questão possui mensagem enviada ao nó defeituoso
                for (Ptr<MessageData> m : evt->GetMSent()){
                    if (m->GetTo() == to){
                        msgsToResend.push_back(m);
                    }
                }
            }

        }
    }

    return msgsToResend;
}

Ptr<EventEARPWRv2> EARPWithoutRollbackV2::searchForEvent(Address to, int msgsSent){
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) 
                << ", " << app->getNodeName() 
                << " buscando evento a partir do qual serão reenviadas mensagens. ");
    
    //Procurando primeiramente nos eventos da memória volátil 
    Ptr<EventEARPWRv2> e = searchForEvent(events, to, msgsSent);

    if (e != nullptr)
        return e;

    //Procurando nos checkpoints
    int lastCpId = checkpointHelper->getLastCheckpointId();
    int cpId = 0;

    while (cpId <= lastCpId){
        //lendo checkpoint
        json j = checkpointHelper->readCheckpoint(cpId);

        //Recupera os eventos presentes no checkpoint
        vector<Ptr<EventEARPWRv2>> eventsToAnalyze = j.get<vector<Ptr<EventEARPWRv2>>>();
        Ptr<EventEARPWRv2> e = searchForEvent(eventsToAnalyze, to, msgsSent);

        if (e != nullptr){
            return e;
        }

        cpId++;
    }

    NS_ABORT_MSG("ERRO: não foi possível estabelecer um evento adequado para o reenvio de mensagens.");
}

Ptr<EventEARPWRv2> EARPWithoutRollbackV2::searchForEvent(vector<Ptr<EventEARPWRv2>> evts, Address to, int msgsSent){
    NS_LOG_FUNCTION(this);
    
    for (size_t i = 0; i < evts.size(); i++){
        
        Ptr<EventEARPWRv2> e = evts.at(i);
        json jNodesInfo = e->GetNodeState().at("checkpointStrategy").at("adjacentNodes");
        vector<Ptr<NodeInfoEARPWR>> nodesInfo = jNodesInfo.get<vector<Ptr<NodeInfoEARPWR>>>();

        //Informações do nó remoto na época do evento "e" 
        Ptr<NodeInfoEARPWR> nodeInfo = findAdjacentNodeByAddress(nodesInfo, to);

        if (nodeInfo == nullptr)
            continue;

        //Verificando se este é o evento buscado
        if (nodeInfo->GetMessagesSent() == msgsSent){
            
            NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) 
                << ", " << app->getNodeName() 
                << " definiu o evento " << e->GetJ() << " para início do reenvio de mensagens.");

            return e;
        }
    }

    return nullptr;
}

void EARPWithoutRollbackV2::requestResendToAdjacentNodes(){
    NS_LOG_FUNCTION(this);

    if (!adjacentNodes.empty()){
        for (Ptr<NodeInfoEARPWR> i : adjacentNodes){
            requestResend(i);
        }
    }
}

void EARPWithoutRollbackV2::requestResend(Ptr<NodeInfoEARPWR> i){
    NS_LOG_FUNCTION(this);
    
    app->send(REQUEST_RESEND, i->GetMessagesReceived(), i->GetAddress());
    resentMsgsMap[i->GetAddress()] = vector<Ptr<MessageData>>();
}

bool EARPWithoutRollbackV2::interceptSend(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);

    if (!app->isAlive()
        || ((md->GetCommand() == REQUEST_VALUE 
            || md->GetCommand() == RESPONSE_VALUE) 
            && rollbackInProgress)){
            
            return true;
    }

    if (md->GetCommand() == REQUEST_VALUE){
        
        if (checkIfDestinationHasFailed(md->GetTo())){
            return true;
        }

        //O envio de uma mensagem de request gera um novo evento
        Ptr<EventEARPWRv2> e = Create<EventEARPWRv2>();
        e->SetNodeState(app->to_json()); //O estado do nó a ser gravado é o de antes do processamento da mensagem
        e->SetJ(++eventCount);
        e->SetMSent(vector<Ptr<MessageData>>{md});

        events.push_back(e);
        updateNodeInfoAfterSend(md);

        return false;
    }

    if (md->GetCommand() == RESPONSE_VALUE){
        
        /* Uma mensagem do tipo response é enviada em decorrência do recebimento de uma
        mensagem anterior. Nesse caso, deve-se atualizar o último evento registrado. */
        Ptr<EventEARPWRv2> e = getActiveEvent();
        e->SetMSent(vector<Ptr<MessageData>>{md});
        
        updateNodeInfoAfterSend(md);

        return false;
    }

    return false;
}

void EARPWithoutRollbackV2::updateNodeInfoAfterReceive(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);
    
    addToAdjacentNodes(md->GetFrom());
    
    //Atualizando informações de quantidade de mensagens recebidas
    Ptr<NodeInfoEARPWR> result = findAdjacentNodeByAddress(md->GetFrom());
    int mReceived = result->GetMessagesReceived();
    result->SetMessagesReceived(mReceived + 1);
}

void EARPWithoutRollbackV2::updateNodeInfoAfterSend(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);
    
    addToAdjacentNodes(md->GetTo());
        
    //Atualizando informações de quantidade de mensagens enviadas
    Ptr<NodeInfoEARPWR> result = findAdjacentNodeByAddress(md->GetTo());
    int mSent = result->GetMessagesSent();
    result->SetMessagesSent(mSent + 1);
}

void EARPWithoutRollbackV2::replayEvent(Ptr<EventEARPWRv2> e){
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) 
                << ", " << app->getNodeName() 
                << " realizando replay das mensagens presentes no evento " 
                << e->GetJ() << "...");

    Ptr<MessageData> msgReceived = e->GetMessage();

    //Se houve alguma mensagem recebida no evento
    if (msgReceived != nullptr && msgReceived->GetCommand() != ""){
        
        //Faz o processamento do recebimento
        updateNodeInfoAfterReceive(msgReceived);
        app->replayReceive(msgReceived, false, false);
    }

    //Se houve alguma mensagem enviada no evento
    if (!e->GetMSent().empty()){
        
        for (Ptr<MessageData> md : e->GetMSent()){
                
            updateNodeInfoAfterSend(md);
            app->send(md->GetCommand(), md->GetData(), md->GetTo(), true);
            
        }
    }

    ++eventCount;
}

bool EARPWithoutRollbackV2::checkIfDestinationHasFailed(Address dest){
    //Este método deve ser utilizado apenas por nós clientes
    //Verificando se a requisição anterior para o nó destino foi respondida
    
    bool foundRequest = false;
    bool foundResponse = false;

    //Procurando último evento no qual houve mensagem enviada para o nó destino ou recebida dele
    for (int i = events.size() - 1; i >= 0; i--){
        Ptr<EventEARPWRv2> e = events.at(i);

        //Verificando se o evento se trata do envio de uma requisição
        if (!e->GetMSent().empty()){
            Ptr<MessageData> mSent = e->GetMSent().at(0);

            if (mSent->GetCommand().find(REQUEST_VALUE) != string::npos && mSent->GetTo() == dest){
                foundRequest = true;
                break;
            }
        }

        //Verificando se o evento se trata do recebimento de uma resposta
        if (e->GetMessage() != nullptr 
            && e->GetMessage()->GetCommand().find(RESPONSE_VALUE) != string::npos
            && e->GetMessage()->GetFrom() == dest){
            
            foundResponse = true;
            break;
        }
    }

    if (foundRequest && !foundResponse){
        return true;
    } else {
        return false;
    }
}

void EARPWithoutRollbackV2::addToAdjacentNodes(Address a){
    NS_LOG_FUNCTION(this);
    
    Ptr<NodeInfoEARPWR> result = findAdjacentNodeByAddress(a);

    if (result == nullptr) {
        
        Ptr<NodeInfoEARPWR> i = CreateObject<NodeInfoEARPWR>(a);

        //Endereço não foi encontrado
        //Adiciona novo elemento ao vetor
        adjacentNodes.push_back(i);
    
    }
}

Ptr<NodeInfoEARPWR> EARPWithoutRollbackV2::findAdjacentNodeByAddress(vector<Ptr<NodeInfoEARPWR>> v, Address addr){
    NS_LOG_FUNCTION(this);
    
    auto it = find_if(v.begin(), v.end(), [&addr](const Ptr<NodeInfoEARPWR> n) {
        return n->GetAddress() == addr;
    });

    if (it != v.end()) {
        return it->operator->(); // Retorna ponteiro para o NodeInfoEARPWR encontrado
    }

    return nullptr; // Não encontrado
}

Ptr<NodeInfoEARPWR> EARPWithoutRollbackV2::findAdjacentNodeByAddress(Address addr) {
    NS_LOG_FUNCTION(this);
    return findAdjacentNodeByAddress(adjacentNodes, addr);
}

Ptr<EventEARPWRv2> EARPWithoutRollbackV2::getActiveEvent(){
    NS_LOG_FUNCTION(this);

    if (events.size() == 0)
        return nullptr;
    
    return events.at(events.size() - 1);
}

void EARPWithoutRollbackV2::logSenderId(Ptr<MessageData> md){
    
    json j = checkpointHelper->readLog(getSenderIdsLogName());
    vector<Address> ids;
    
    if (!j.empty())
        ids = j;
    
    ids.push_back(md->GetFrom());
    checkpointHelper->writeLog(getSenderIdsLogName(), ids, true);

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", log de IDs atualizado por " 
            << app->getNodeName() << ".");

    app->decreaseCheckpointEnergy();
}

string EARPWithoutRollbackV2::getSenderIdsLogName(){
    return app->getNodeName() + "-log-id.json";
}

int EARPWithoutRollbackV2::GetTotalMsgsReceived(){
    int totalMsgs = 0;
    
    if (!adjacentNodes.empty()){
        for (Ptr<NodeInfoEARPWR> n : adjacentNodes){
            totalMsgs += n->GetMessagesReceived();
        }
    }

    return totalMsgs;
}

void EARPWithoutRollbackV2::printData() {
    NS_LOG_INFO("rec_i = " << rec_i 
                << ", csn = " << csn
                << ", adjacentNodes.size() = " << adjacentNodes.size() 
                << ", events.size() = " << events.size()
                << ", eventCount = " << eventCount
                << ", checkpointId = " << checkpointId
                << "\n");
}

void to_json(json& j, const Ptr<EventEARPWRv2>& obj) {
    NS_LOG_FUNCTION("to_json(json& j, const Ptr<EventEARPWRv2>& obj)");
    
    j = json {
        {"j", obj->j},
        {"nodeState", obj->nodeState},
        {"m", obj->m},
        {"mSent", obj->mSent},
    };
}

void from_json(const json& j, Ptr<EventEARPWRv2>& obj) {
    NS_LOG_FUNCTION("from_json(const json& j, Ptr<EventEARPWRv2>& obj)");

    obj = CreateObject<EventEARPWRv2>();
    j.at("j").get_to(obj->j);
    j.at("nodeState").get_to(obj->nodeState);
    j.at("m").get_to(obj->m);
    j.at("mSent").get_to(obj->mSent);
}

json EARPWithoutRollbackV2::to_json() {
    NS_LOG_FUNCTION("EARPWithoutRollbackV2::to_json");
    
    json j = CheckpointStrategy::to_json();
    j["adjacentNodes"] = adjacentNodes;
    j["eventCount"] = eventCount;
    j["csn"] = csn;
    j["totalNodesQuantity"] = totalNodesQuantity;
    
    return j;
}

void EARPWithoutRollbackV2::from_json(const json& j) {
    NS_LOG_FUNCTION("EARPWithoutRollbackV2::from_json");
    
    CheckpointStrategy::from_json(j);
    j.at("adjacentNodes").get_to(adjacentNodes);
    j.at("eventCount").get_to(eventCount);
    j.at("csn").get_to(csn);
    j.at("totalNodesQuantity").get_to(totalNodesQuantity);
}

} // Namespace ns3
