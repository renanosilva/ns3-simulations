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

#include "efficient-assync-recovery-protocol.h"
#include "ns3/server-node-app.h"
#include "ns3/simulator.h"
#include "ns3/node-depleted-exception.h"
#include "ns3/node-asleep-exception.h"
#include "ns3/log-utils.h"
#include "ns3/application-type.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EfficientAssyncRecoveryProtocol");

NS_OBJECT_ENSURE_REGISTERED(EfficientAssyncRecoveryProtocol);

TypeId
EfficientAssyncRecoveryProtocol::GetTypeId()
{
    NS_LOG_FUNCTION(EfficientAssyncRecoveryProtocol::GetTypeId());

    static TypeId tid =
        TypeId("ns3::EfficientAssyncRecoveryProtocol")
            .SetParent<CheckpointStrategy>()
            .SetGroupName("Checkpoint")
            .AddConstructor<EfficientAssyncRecoveryProtocol>()
            .AddAttribute("interval",
                          "Intervalo de tempo no qual serão criados checkpoints.",
                          TimeValue(Seconds(5.0)), //valor inicial
                          MakeTimeAccessor(&EfficientAssyncRecoveryProtocol::interval),
                          MakeTimeChecker());
    
    return tid;
}

EfficientAssyncRecoveryProtocol::EfficientAssyncRecoveryProtocol(Time timeInterval, int nodesQuantity, Ptr<CheckpointApp> application){
    NS_LOG_FUNCTION(this);

    interval = timeInterval;
    rec_i = -1;
    eventCount = -1;
    app = application;
    creationScheduling = EventId();
    checkpointHelper = Create<CheckpointHelper>(application->getNodeName());
    adjacentNodes.clear();
    events.clear();
    csn = -1;
    totalNodesQuantity = nodesQuantity;
    rollbackInProgress = false;
    rollbackIteration = -1;
    rollbackMsgsToProcess.clear();
}

EfficientAssyncRecoveryProtocol::EfficientAssyncRecoveryProtocol(){
    NS_LOG_FUNCTION(this);
    rec_i = -1;
    eventCount = -1;
    adjacentNodes.clear();
    events.clear();
    csn = -1;
    rollbackInProgress = false;
    rollbackIteration = -1;
    rollbackMsgsToProcess.clear();
}

EfficientAssyncRecoveryProtocol::~EfficientAssyncRecoveryProtocol()
{
    NS_LOG_FUNCTION(this);
    
    adjacentNodes.clear();
    events.clear();
    rollbackMsgsToProcess.clear();

    stopCheckpointing();
}

void EfficientAssyncRecoveryProtocol::startCheckpointing() {
    NS_LOG_FUNCTION(this);

    //Agendando a criação do checkpoint para logo após o início da aplicação
    // Time delay = Time("0.000000000000000000000001s");
    // creationScheduling = Simulator::Schedule(delay, &EfficientAssyncRecoveryProtocol::writeCheckpoint, this);

    scheduleNextCheckpoint();
}

void EfficientAssyncRecoveryProtocol::stopCheckpointing() {
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(creationScheduling);
}

void EfficientAssyncRecoveryProtocol::writeCheckpoint() {
    NS_LOG_FUNCTION(this);

    if (!app->mayCheckpoint() || events.empty()){
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

Time EfficientAssyncRecoveryProtocol::getDelayToNextCheckpoint(){
    NS_LOG_FUNCTION(this);
    
    // double now = Simulator::Now().GetSeconds();
    // double intervalSec = interval.GetSeconds();
    // double mod = std::fmod(now, intervalSec);
    // double nextCheckpointing = intervalSec - mod;
    // Time delay = Time(to_string(nextCheckpointing) + "s");
    
    // return delay;

    return interval;
}

void EfficientAssyncRecoveryProtocol::scheduleNextCheckpoint(){
    NS_LOG_FUNCTION(this);

    NS_LOG_LOGIC("\n" << checkpointHelper->getCheckpointBasename() << " agendando próximo checkpoint\n");

    //Agendando próximo checkpoint
    Time delay = getDelayToNextCheckpoint();

    //Será agendado com um delay calculado para garantir o intervalo de tempo predefinido
    creationScheduling = Simulator::Schedule(delay,
                                &EfficientAssyncRecoveryProtocol::writeCheckpoint,
                                this);
}

void EfficientAssyncRecoveryProtocol::rollbackToLastVolatileEvent() {
    NS_LOG_FUNCTION(this);
    
    /* Fazendo rollback para o último evento do log volátil, já que este nó não falhou 
    (apenas tomou conhecimento da falha de outro nó) */
    Ptr<EventEARP> e = getActiveEvent();

    if (e != nullptr)
        rollbackToVolatileEvent(e, true);
    else {
        
        //não existem eventos registrados no checkpoint atual ainda
        //nesse caso, faz-se um rollback para o último checkpoint

        rollbackInProgress = true;
        rollbackIteration = 0;
        app->StopApplication();
        rollback(getLastCheckpointId());

        //Enviando mensagem de requisição de rollback para os nós adjacentes
        notifyNodesAboutRollback();
    }
}

void EfficientAssyncRecoveryProtocol::rollbackToVolatileEvent(Ptr<EventEARP> e, bool notifyNodes) {
    rollbackInProgress = true;

    app->StopApplication();
    app->beforeRollback();

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << app->getNodeName() 
                        << " iniciando procedimento de ROLLBACK para evento da memória volátil (" 
                        << e->GetJ() << ").");
    
    //lendo o estado do sistema referente ao evento em questão
    json j = e->GetNodeState();
    
    //Recupera o estado da aplicação presente no evento
    app->from_json(j);
    
    //Atualizando o ponto de recuperação do sistema
    rec_i = e->GetJ();
    
    app->afterRollback();

    /* Fazendo o replay das mensagens presentes no evento carregado.
    Isso é necessário, pois o estado é referente ao momento imediatamente 
    anterior ao evento e o protocolo diz que, quando o rollback é feito,
    o sistema deve ficar no estado posterior ao processamento das
    mensagens do evento. */  
    
    replayEvent(e);

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << app->getNodeName() 
            << " concluiu o procedimento de rollback.");

    // Como o rollback é feito utilizando a memória volátil, não há consumo extra de energia
    // app->decreaseRollbackEnergy();

    NS_LOG_INFO("\nDepois do rollback...");
    app->printNodeData();

    //Se o nó não tiver descarregado durante o processo
    if (!app->isDepleted() && !app->isSleeping() && notifyNodes){
        notifyNodesAboutRollback(); 
    }
}

void EfficientAssyncRecoveryProtocol::rollbackToLastCheckpoint() {
    NS_LOG_FUNCTION(this);   
    
    try {
        rollbackInProgress = true;
        rollbackIteration = -1;
        rollback(getLastCheckpointId());

        //Enviando mensagem de aviso de falha para os nós adjacentes
        notifyNodesAboutRollback();

        rollbackIteration++;

        //Enviando mensagem de requisição de rollback para os nós adjacentes
        notifyNodesAboutRollback();
    } catch (NodeAsleepException& e) {
        NS_LOG_LOGIC("Tarefa incompleta por estar em modo SLEEP.");
        return;
    } catch (NodeDepletedException& e) {
        NS_LOG_LOGIC("Tarefa incompleta por estar em modo DEPLETED.");
        return;
    }
}

bool EfficientAssyncRecoveryProtocol::rollback(int cpId) {
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
    events = j.get<vector<Ptr<EventEARP>>>();
    
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

void EfficientAssyncRecoveryProtocol::notifyNodesAboutRollback(){
    NS_LOG_FUNCTION(this);

    // Enviando notificação para os nós com os quais houve comunicação
    if (adjacentNodes.size() > 0){

        for (EARPNodeInfo i : adjacentNodes){
            
            /* Enviando solicitação de rollback para o destino. Se for apenas uma solicitação inicial de rollback,
            (aviso de falha), não é enviado o valor de SENTi->j, e sim um valor simbólico de -1. */
            
            Ptr<MessageData> md = app->send(REQUEST_TO_START_ROLLBACK_COMMAND, 
                (rollbackIteration == -1 ? -1 : i.GetMessagesSent()), 
                // i.GetMessagesSent(), 
                i.GetAddress());

        }
    }
}

void EfficientAssyncRecoveryProtocol::notifyNodesAboutRollbackExcept(EARPNodeInfo exceptNode){
    NS_LOG_FUNCTION(this);

    // Enviando notificação para os nós com os quais houve comunicação
    if (adjacentNodes.size() > 0){
        for (EARPNodeInfo i : adjacentNodes){
            
            if (i.GetAddress() == exceptNode.GetAddress())
                continue;

            /* Enviando solicitação de rollback para o destino. Se for apenas uma solicitação inicial de rollback,
            (aviso de falha), não é enviado o valor de SENTi->j, e sim um valor simbólico de -1. */
            
            Ptr<MessageData> md = app->send(REQUEST_TO_START_ROLLBACK_COMMAND, 
                (rollbackIteration == -1 ? -1 : i.GetMessagesSent()), 
                // i.GetMessagesSent(), 
                i.GetAddress());

        }
    }
}

bool EfficientAssyncRecoveryProtocol::interceptRead(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this << md);
    
    try {
        if ((md->GetCommand() == REQUEST_VALUE || md->GetCommand() == RESPONSE_VALUE) && rollbackInProgress)
            return true;

        if (md->GetCommand() == REQUEST_VALUE){
            
            //O recebimento de uma mensagem de request gera um novo evento
            Ptr<EventEARP> e = Create<EventEARP>();
            e->SetNodeState(app->to_json()); //O estado do nó a ser gravado é o de antes do processamento da mensagem
            e->SetJ(++eventCount);
            e->SetMessage(*md);

            events.push_back(e);
            processMessageReceived(md);

            return false;
        }
        
        if (md->GetCommand() == RESPONSE_VALUE){
            
            //O recebimento de uma mensagem de response gera um novo evento (sem novos envios, ou seja, com mSent vazio)
            Ptr<EventEARP> e = Create<EventEARP>();
            e->SetNodeState(app->to_json()); //O estado do nó a ser gravado é o de antes do processamento da mensagem
            e->SetJ(++eventCount);
            e->SetMessage(*md);

            events.push_back(e);
            processMessageReceived(md);

            return false;
        }

        if (md->GetCommand() == REQUEST_TO_START_ROLLBACK_COMMAND){
            utils::logMessageReceived(app->getNodeName(), md);
            app->decreaseReadEnergy();

            if (!rollbackInProgress || md->GetData() == -1){
                
                /* A mensagem recebida é apenas uma sinalização de falha do nó emissor. 
                Primeira iteração: é feito um rollback para o último evento do log volátil,
                após tomar conhecimento da falha de outro nó. */ 

                //Avisando outros nós sobre o (re)início do processo de rollback
                rollbackIteration = -1;
                notifyNodesAboutRollbackExcept(md->GetFrom());
                
                rollbackMsgsToProcess.clear();
                checkpointId = getLastCheckpointId() + 1;
                rollbackIteration = 0;
                rollbackToLastVolatileEvent();

            } else {
                /* Trata-se de mensagem referente a uma das iterações de rollback. 
                A mensagem recebida é adicionada à fila de processamento. */
                rollbackMsgsToProcess.push_back(md);
            }
                    
            //Se já tiver recebido uma mensagem de rollback de cada nó adjacente
            if (allRollbackMsgReceived()){
                
                processRollbackMsgsQueue();

                rollbackIteration++;

                NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) 
                    << ", " << app->getNodeName() 
                    << " avançou para a iteração " << rollbackIteration << ".");

                if (rollbackIteration >= totalNodesQuantity) {
                    
                    //Rollback concluído
                    rollbackInProgress = false;
                    rollbackIteration = -1;

                    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) 
                        << ", " << app->getNodeName() 
                        << " concluiu todas as iterações de rollback com rec_i = "
                        << rec_i << ".");

                } else {
                    notifyNodesAboutRollback();
                }
            }

            return true;
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

bool EfficientAssyncRecoveryProtocol::interceptSend(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);

    //Ignora mensagens de aplicação caso um rollback esteja em andamento
    if ((md->GetCommand() == REQUEST_VALUE || md->GetCommand() == RESPONSE_VALUE) && rollbackInProgress)
        return true;

    if (md->GetCommand() == REQUEST_VALUE){
        
        //O envio de uma mensagem de request gera um novo evento
        Ptr<EventEARP> e = Create<EventEARP>();
        e->SetNodeState(app->to_json()); //O estado do nó a ser gravado é o de antes do processamento da mensagem
        e->SetJ(++eventCount);
        e->SetMSent(vector<MessageData>{*md});

        events.push_back(e);
        processMessageSent(md);
    }

    if (md->GetCommand() == RESPONSE_VALUE){
        
        /* Uma mensagem do tipo response é enviada em decorrência do recebimento de uma
        mensagem anterior. Nesse caso, deve-se atualizar o último evento registrado. */
        Ptr<EventEARP> e = getActiveEvent();
        e->SetMSent(vector<MessageData>{*md});
        
        processMessageSent(md);

    }

    return false;
}

bool EfficientAssyncRecoveryProtocol::allRollbackMsgReceived(){
    
    for (EARPNodeInfo i : adjacentNodes){
        
        //Indica se recebeu mensagem de rollback do nó da iteração atual
        bool received = false;

        for (Ptr<MessageData> md : rollbackMsgsToProcess){
            if (md->GetFrom() == i.GetAddress()){
                received = true;
            }
        } 

        if (!received)
            return false;
    }

    return true;

}

void EfficientAssyncRecoveryProtocol::processMessageReceived(Ptr<MessageData> md){
    addToAdjacentNodes(md->GetFrom());
        
    //Atualizando informações de quantidade de mensagens recebidas
    EARPNodeInfo* result = findAdjacentNodeByAddress(md->GetFrom());
    int mReceived = result->GetMessagesReceived();
    result->SetMessagesReceived(mReceived + 1);
}

void EfficientAssyncRecoveryProtocol::processMessageSent(Ptr<MessageData> md){
    addToAdjacentNodes(md->GetTo());
        
    //Atualizando informações de quantidade de mensagens enviadas
    EARPNodeInfo* result = findAdjacentNodeByAddress(md->GetTo());
    int mSent = result->GetMessagesSent();
    result->SetMessagesSent(mSent + 1);
}

void EfficientAssyncRecoveryProtocol::processRollbackMsgsQueue(){
    if (!rollbackMsgsToProcess.empty()){

        //Percorrendo a lista de mensagens de rollback a serem processadas
        for (Ptr<MessageData> md : rollbackMsgsToProcess){
            
            //Obtendo informações do nó emissor referente ao evento atual
            EARPNodeInfo* node = findAdjacentNodeByAddress(md->GetFrom()); 

            //Verificando se é necessário fazer rollback
            if (node->GetMessagesReceived() > md->GetData()){
                
                /* Nesse caso, deve-se realizar rollback para o último evento tal que a quantidade
                de mensagens recebidas seja igual à quantidade indicada na mensagem. */

                bool eventFound = false;

                while (!eventFound){

                    //Percorrendo os eventos do checkpoint atual 
                    for (size_t i = 0; i < events.size(); i++){
                        
                        Ptr<EventEARP> e = events.at(i);
                        json jNodesInfo = e->GetNodeState().at("checkpointStrategy").at("adjacentNodes");
                        vector<EARPNodeInfo> nodesInfo = jNodesInfo.get<vector<EARPNodeInfo>>();

                        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) 
                            << ", " << app->getNodeName() 
                            << " analisando rollback para o evento " << e->GetJ() << ".");

                        //Informações do nó que mandou a mensagem de rollback na época do evento "e" 
                        EARPNodeInfo* result = findAdjacentNodeByAddress(&nodesInfo, md->GetFrom());

                        if (result == nullptr)
                            continue;

                        //Verificando se este é o evento buscado para rollback
                        //Se a quantidade de mensagens recebidas for igual à quantidade de mensagens enviadas pelo outro nó
                        //e não tiver havido recebimento de mensagem no evento (já que ela passaria por replay)
                        //OU se a quantidade de mensagens recebidas for igual à quantidade de mensagens enviadas pelo outro nó
                        //menos uma e houve recebimento de mensagem no evento (passando a ficar igual após o replay)
                        if ((result->GetMessagesReceived() == md->GetData() && e->GetMessage().GetCommand() == "")
                            || (result->GetMessagesReceived() == (md->GetData() - 1) && e->GetMessage().GetCommand() != "")){
                            
                            //O evento buscado foi encontrado. Deve-se fazer rollback para este evento (caso já não esteja nele).
                            eventFound = true;
                            
                            NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) 
                                << ", " << app->getNodeName() 
                                << " definiu o evento " << e->GetJ() << " para rollback.");

                            if (rec_i != e->GetJ()){
                                rollbackToVolatileEvent(e, false);
                            } 

                            break;
                        }
                    }
                    
                    //Se o evento não tiver sido encontrado e ainda existir checkpoint anterior
                    if (!eventFound && (checkpointId > 0 || (checkpointId == 0 && rec_i > getActiveEvent()->GetJ()))){
                        
                        app->StopApplication();

                        //Faz rollback para o checkpoint anterior 
                        rollback(checkpointId - 1);
                    
                    } else if (!eventFound){
                        NS_ABORT_MSG("ERRO: não foi possível estabelecer um evento adequado para rollback.");
                    }

                }
            } else {
                NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) 
                    << ", " << app->getNodeName() 
                    << " não realizou rollback, pois já está no evento adequado. ");
            }
        }

        rollbackMsgsToProcess.clear();

    }
}

void EfficientAssyncRecoveryProtocol::replayEvent(Ptr<EventEARP> e){
    
    MessageData msgReceived = e->GetMessage();

    //Se houve alguma mensagem recebida no evento
    if (msgReceived.GetCommand() != ""){
        
        //Faz o processamento do recebimento
        
        addToAdjacentNodes(msgReceived.GetFrom());

        //Atualizando informações de quantidade de mensagens recebidas
        EARPNodeInfo* result = findAdjacentNodeByAddress(msgReceived.GetFrom());
        int quantReceived = result->GetMessagesReceived();
        result->SetMessagesReceived(quantReceived + 1);

        Ptr<MessageData> md = CreateObject<MessageData>();
        md->SetCommand(msgReceived.GetCommand());
        md->SetData(msgReceived.GetData());
        md->SetFrom(msgReceived.GetFrom());
        md->SetPiggyBackedInfo(msgReceived.GetPiggyBackedInfo());
        md->SetSequenceNumber(msgReceived.GetSequenceNumber());
        md->SetSize(msgReceived.GetSize());
        md->SetTo(msgReceived.GetTo());
        md->SetUid(msgReceived.GetUid());
        
        app->replayReceive(md, false, false);
    }

    //Se houve alguma mensagem enviada no evento
    if (!e->GetMSent().empty()){
        
        for (MessageData md : e->GetMSent()){
                
            addToAdjacentNodes(md.GetTo());
        
            //Atualizando informações de quantidade de mensagens enviadas
            EARPNodeInfo* result = findAdjacentNodeByAddress(md.GetTo());
            int mSent = result->GetMessagesSent();
            result->SetMessagesSent(mSent + 1);

            app->send(md.GetCommand(), md.GetData(), md.GetTo(), true);
        }
    }

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) 
                << ", " << app->getNodeName() 
                << " realizou replay das mensagens presentes no evento " 
                << e->GetJ() << ".");
}

void EfficientAssyncRecoveryProtocol::addToAdjacentNodes(EARPNodeInfo i){
    NS_LOG_FUNCTION(this);
    
    EARPNodeInfo* result = findAdjacentNodeByAddress(i.GetAddress());

    if (result == nullptr) {
        
        //Endereço não foi encontrado
        //Adiciona novo elemento ao vetor
        adjacentNodes.push_back(i);
    
    }
}

EARPNodeInfo* EfficientAssyncRecoveryProtocol::findAdjacentNodeByAddress(vector<EARPNodeInfo>* v, const Address& addr){
    auto it = find_if(v->begin(), v->end(), [&addr](const EARPNodeInfo& n) {
        return n.GetAddress() == addr;
    });

    if (it != v->end()) {
        return &(*it); // Retorna ponteiro para o EARPNodeInfo encontrado
    }

    return nullptr; // Não encontrado
}

EARPNodeInfo* EfficientAssyncRecoveryProtocol::findAdjacentNodeByAddress(const Address& addr) {
    return findAdjacentNodeByAddress(&adjacentNodes, addr);
}

Ptr<EventEARP> EfficientAssyncRecoveryProtocol::getActiveEvent(){
    if (events.size() == 0)
        return nullptr;
    
    return events.at(events.size() - 1);
}

void EfficientAssyncRecoveryProtocol::printData() {
    NS_LOG_INFO("rec_i = " << rec_i 
                << ", csn = " << csn
                << ", adjacentNodes.size() = " << adjacentNodes.size() 
                << ", events.size() = " << events.size()
                << ", checkpointId = " << checkpointId
                << "\n");
}

void to_json(json& j, const EARPNodeInfo& obj) {
    j = json{
        {"address", obj.address},
        {"messagesSent", obj.messagesSent},
        {"messagesReceived", obj.messagesReceived}
    };
}

void from_json(const json& j, EARPNodeInfo& obj) {
    j.at("address").get_to(obj.address);
    j.at("messagesSent").get_to(obj.messagesSent);
    j.at("messagesReceived").get_to(obj.messagesReceived);
}

// void to_json(json& j, const Event& obj) {
//     j = json{
//         {"j", obj.j},
//         {"nodeState", obj.nodeState},
//         {"m", obj.m},
//         {"mSent", obj.mSent},
//     };
// }

// void from_json(const json& j, Event& obj) {
//     j.at("j").get_to(obj.j);
//     j.at("nodeState").get_to(obj.nodeState);
//     j.at("m").get_to(obj.m);
//     j.at("mSent").get_to(obj.mSent);
// }

void to_json(json& j, const Ptr<EventEARP>& obj) {
    j = json{
        {"j", obj->j},
        {"nodeState", obj->nodeState},
        {"m", obj->m},
        {"mSent", obj->mSent},
    };
}

void from_json(const json& j, Ptr<EventEARP>& obj) {
    obj = CreateObject<EventEARP>();
    j.at("j").get_to(obj->j);
    j.at("nodeState").get_to(obj->nodeState);
    j.at("m").get_to(obj->m);
    j.at("mSent").get_to(obj->mSent);
}

json EfficientAssyncRecoveryProtocol::to_json() {
    NS_LOG_FUNCTION("EfficientAssyncRecoveryProtocol::to_json");
    
    json j = CheckpointStrategy::to_json();
    j["adjacentNodes"] = adjacentNodes;
    j["eventCount"] = eventCount;
    j["csn"] = csn;
    j["totalNodesQuantity"] = totalNodesQuantity;
    
    return j;
}

void EfficientAssyncRecoveryProtocol::from_json(const json& j) {
    NS_LOG_FUNCTION("EfficientAssyncRecoveryProtocol::from_json");
    
    CheckpointStrategy::from_json(j);
    j.at("adjacentNodes").get_to(adjacentNodes);
    j.at("eventCount").get_to(eventCount);
    j.at("csn").get_to(csn);
    j.at("totalNodesQuantity").get_to(totalNodesQuantity);
}

} // Namespace ns3
