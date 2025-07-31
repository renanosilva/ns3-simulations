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

#include "earp-without-rollback-v2-optimized.h"
#include "ns3/server-node-app.h"
#include "ns3/simulator.h"
#include "ns3/node-depleted-exception.h"
#include "ns3/node-asleep-exception.h"
#include "ns3/log-utils.h"
#include "ns3/application-type.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EARPWithoutRollbackV2Optimized");

NS_OBJECT_ENSURE_REGISTERED(EARPWithoutRollbackV2Optimized);
NS_OBJECT_ENSURE_REGISTERED(EventEARPWRv2);

TypeId
EARPWithoutRollbackV2Optimized::GetTypeId()
{
    NS_LOG_FUNCTION(EARPWithoutRollbackV2Optimized::GetTypeId());

    static TypeId tid =
        TypeId("ns3::EARPWithoutRollbackV2Optimized")
            .SetParent<CheckpointStrategy>()
            .SetGroupName("Checkpoint")
            .AddConstructor<EARPWithoutRollbackV2Optimized>()
            .AddAttribute("interval",
                          "Intervalo de tempo no qual serão criados checkpoints.",
                          TimeValue(Seconds(5.0)), //valor inicial
                          MakeTimeAccessor(&EARPWithoutRollbackV2Optimized::interval),
                          MakeTimeChecker());
    
    return tid;
}

EARPWithoutRollbackV2Optimized::EARPWithoutRollbackV2Optimized(Time timeInterval, int nodesQuantity, Ptr<CheckpointApp> application){
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

EARPWithoutRollbackV2Optimized::EARPWithoutRollbackV2Optimized(){
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

EARPWithoutRollbackV2Optimized::~EARPWithoutRollbackV2Optimized()
{
    NS_LOG_FUNCTION(this);
}

void EARPWithoutRollbackV2Optimized::DisposeReferences(){
    stopCheckpointing();
    
    adjacentNodes.clear();
    events.clear();
    senderIds.clear();
    outIds.clear();
    resentMsgsMap.clear();
    failedNodes.clear();
}

void EARPWithoutRollbackV2Optimized::startCheckpointing() {
    NS_LOG_FUNCTION(this);
    scheduleNextCheckpoint();
}

void EARPWithoutRollbackV2Optimized::stopCheckpointing() {
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(creationScheduling);
}

void EARPWithoutRollbackV2Optimized::writeCheckpoint() {
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

Time EARPWithoutRollbackV2Optimized::getDelayToNextCheckpoint(){
    NS_LOG_FUNCTION(this);
    
    return interval;
}

void EARPWithoutRollbackV2Optimized::scheduleNextCheckpoint(){
    NS_LOG_FUNCTION(this);

    //Agendando próximo checkpoint
    Time delay = getDelayToNextCheckpoint();

    //Será agendado com um delay calculado para garantir o intervalo de tempo predefinido
    creationScheduling = Simulator::Schedule(delay,
                                &EARPWithoutRollbackV2Optimized::writeCheckpoint,
                                Ptr<EARPWithoutRollbackV2Optimized>(this));
}

void EARPWithoutRollbackV2Optimized::rollbackToLastCheckpoint() {
    NS_LOG_FUNCTION(this);   
    
    try {
        //Iniciando rollback
        rollbackInProgress = true;
        rollback(getLastCheckpointId());

        //Reseta os eventos da memória volátil pois eles já estão registrados em checkpoint
        events.clear();
        rollbackInProgress = false;

    } catch (NodeAsleepException& e) {
        NS_LOG_LOGIC("Tarefa incompleta por estar em modo SLEEP.");
        return;
    } catch (NodeDepletedException& e) {
        NS_LOG_LOGIC("Tarefa incompleta por estar em modo DEPLETED.");
        return;
    }
}

bool EARPWithoutRollbackV2Optimized::rollback(int cpId) {
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

bool EARPWithoutRollbackV2Optimized::interceptRead(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this << md);

    try {
        if (!app->isAlive()
            || ((md->GetCommand() == REQUEST_VALUE 
                || md->GetCommand() == RESPONSE_VALUE) 
                && rollbackInProgress)){
            
            return true;
        }

        if (md->GetCommand() == REQUEST_VALUE){
            // logSenderId(md);
            return interceptRequestValueReceive(md);
        }
        
        if (md->GetCommand() == RESPONSE_VALUE){
            // logSenderId(md);
            return interceptResponseValueReceive(md);
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

bool EARPWithoutRollbackV2Optimized::interceptRequestValueReceive(Ptr<MessageData> md){
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

bool EARPWithoutRollbackV2Optimized::interceptResponseValueReceive(Ptr<MessageData> md){
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

bool EARPWithoutRollbackV2Optimized::interceptSend(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);

    if (!app->isAlive()
        || ((md->GetCommand() == REQUEST_VALUE 
            || md->GetCommand() == RESPONSE_VALUE) 
            && rollbackInProgress)){
            
            return true;
    }

    if (md->GetCommand() == REQUEST_VALUE){
        
        // if (checkIfDestinationHasFailed(md->GetTo())){
        //     return true;
        // }

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

void EARPWithoutRollbackV2Optimized::updateNodeInfoAfterReceive(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);
    
    addToAdjacentNodes(md->GetFrom());
    
    //Atualizando informações de quantidade de mensagens recebidas
    Ptr<NodeInfoEARPWR> result = findAdjacentNodeByAddress(md->GetFrom());
    int mReceived = result->GetMessagesReceived();
    result->SetMessagesReceived(mReceived + 1);
}

void EARPWithoutRollbackV2Optimized::updateNodeInfoAfterSend(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);
    
    addToAdjacentNodes(md->GetTo());
        
    //Atualizando informações de quantidade de mensagens enviadas
    Ptr<NodeInfoEARPWR> result = findAdjacentNodeByAddress(md->GetTo());
    int mSent = result->GetMessagesSent();
    result->SetMessagesSent(mSent + 1);
}

void EARPWithoutRollbackV2Optimized::replayEvent(Ptr<EventEARPWRv2> e){
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

bool EARPWithoutRollbackV2Optimized::checkIfDestinationHasFailed(Address dest){
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

void EARPWithoutRollbackV2Optimized::addToAdjacentNodes(Address a){
    NS_LOG_FUNCTION(this);
    
    Ptr<NodeInfoEARPWR> result = findAdjacentNodeByAddress(a);

    if (result == nullptr) {
        
        Ptr<NodeInfoEARPWR> i = CreateObject<NodeInfoEARPWR>(a);

        //Endereço não foi encontrado
        //Adiciona novo elemento ao vetor
        adjacentNodes.push_back(i);
    
    }
}

Ptr<NodeInfoEARPWR> EARPWithoutRollbackV2Optimized::findAdjacentNodeByAddress(vector<Ptr<NodeInfoEARPWR>> v, Address addr){
    NS_LOG_FUNCTION(this);
    
    auto it = find_if(v.begin(), v.end(), [&addr](const Ptr<NodeInfoEARPWR> n) {
        return n->GetAddress() == addr;
    });

    if (it != v.end()) {
        return it->operator->(); // Retorna ponteiro para o NodeInfoEARPWR encontrado
    }

    return nullptr; // Não encontrado
}

Ptr<NodeInfoEARPWR> EARPWithoutRollbackV2Optimized::findAdjacentNodeByAddress(Address addr) {
    NS_LOG_FUNCTION(this);
    return findAdjacentNodeByAddress(adjacentNodes, addr);
}

Ptr<EventEARPWRv2> EARPWithoutRollbackV2Optimized::getActiveEvent(){
    NS_LOG_FUNCTION(this);

    if (events.size() == 0)
        return nullptr;
    
    return events.at(events.size() - 1);
}

int EARPWithoutRollbackV2Optimized::GetTotalMsgsReceived(){
    int totalMsgs = 0;
    
    if (!adjacentNodes.empty()){
        for (Ptr<NodeInfoEARPWR> n : adjacentNodes){
            totalMsgs += n->GetMessagesReceived();
        }
    }

    return totalMsgs;
}

void EARPWithoutRollbackV2Optimized::printData() {
    NS_LOG_INFO("rec_i = " << rec_i 
                << ", csn = " << csn
                << ", adjacentNodes.size() = " << adjacentNodes.size() 
                << ", events.size() = " << events.size()
                << ", eventCount = " << eventCount
                << ", checkpointId = " << checkpointId
                << "\n");
}

json EARPWithoutRollbackV2Optimized::to_json() {
    NS_LOG_FUNCTION("EARPWithoutRollbackV2Optimized::to_json");
    
    json j = CheckpointStrategy::to_json();
    j["adjacentNodes"] = adjacentNodes;
    j["eventCount"] = eventCount;
    j["csn"] = csn;
    j["totalNodesQuantity"] = totalNodesQuantity;
    
    return j;
}

void EARPWithoutRollbackV2Optimized::from_json(const json& j) {
    NS_LOG_FUNCTION("EARPWithoutRollbackV2Optimized::from_json");
    
    CheckpointStrategy::from_json(j);
    j.at("adjacentNodes").get_to(adjacentNodes);
    j.at("eventCount").get_to(eventCount);
    j.at("csn").get_to(csn);
    j.at("totalNodesQuantity").get_to(totalNodesQuantity);
}

} // Namespace ns3
