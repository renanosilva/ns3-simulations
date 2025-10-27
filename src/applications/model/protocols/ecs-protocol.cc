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

#include "ecs-protocol.h"
#include "ns3/utils.h"
#include "ns3/server-node-app.h"
#include "ns3/simulator.h"
#include "ns3/node-depleted-exception.h"
#include "ns3/node-asleep-exception.h"
#include "ns3/log-utils.h"
#include "ns3/application-type.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ECSProtocol");
NS_OBJECT_ENSURE_REGISTERED(MessageSenderData);
NS_OBJECT_ENSURE_REGISTERED(EventRecord);

TypeId
ECSProtocol::GetTypeId()
{
    NS_LOG_FUNCTION(ECSProtocol::GetTypeId());

    static TypeId tid =
        TypeId("ns3::ECSProtocol")
            .SetParent<CheckpointStrategy>()
            .SetGroupName("Checkpoint")
            .AddConstructor<ECSProtocol>();
    
    return tid;
}

ECSProtocol::ECSProtocol(Ptr<CheckpointApp> application){
    NS_LOG_FUNCTION(this);

    app = application;
    checkpointHelper = Create<CheckpointHelper>(application->getNodeName());
    failedNodes.clear();
    rollbackInProgress = false;
}

ECSProtocol::ECSProtocol(){
    NS_LOG_FUNCTION(this);
    failedNodes.clear();
    rollbackInProgress = false;
}

ECSProtocol::~ECSProtocol()
{
    NS_LOG_FUNCTION(this);
}

void ECSProtocol::DisposeReferences(){
    stopCheckpointing();
    failedNodes.clear();
}

bool ECSProtocol::interceptRead(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this << md);

    if (md->GetCommand() == REQUEST_VALUE){
        return processRequestSendEvent(md);
    }
    
    if (md->GetCommand() == RESPONSE_VALUE){
        return processResponseSendEvent(md);
    }
    
    return false;
}

bool ECSProtocol::processRequestSendEvent(Ptr<MessageData> md){
    
    //Verificando se a mensagem de requisição possui acks embutidos
    //Primeiro se verifica os acks para registrar eventos anteriores que foram confirmados
    checkForAcksAndLogEvents(md);

    //Registrando numeração de evento do nó emissor
    int eventCount = stoi(md->getPiggyBackedValue("eventCount"));
    Ipv4Address senderAddress = utils::convertAddressToIpv4Address(md->GetFrom());
    eventCountMap[senderAddress].value = eventCount;

    //Criando evento a ser registrado
    Ptr<EventRecord> e = Create<EventRecord>();
    e->SetJ(eventCount);
    e->SetMSent(vector<Ptr<MessageData>>{md});

    // json newEvent = json{};
    // newEvent.push_back(e);
    
    json fileContent = checkpointHelper->readLog(getEventLogFileName(md->GetFrom())); //Lendo eventos já existentes no log
    fileContent.push_back(e); //Adicionando o evento novo

    //Gravando log
    checkpointHelper->writeLog(getEventLogFileName(md->GetFrom()), fileContent, true);

    return false;
}

bool ECSProtocol::processResponseSendEvent(Ptr<MessageData> md){
    
    checkForAcksAndLogEvents(md);

    //Registrando numeração de evento do nó emissor
    int eventCount = stoi(md->getPiggyBackedValue("eventCount"));
    Ipv4Address senderAddress = utils::convertAddressToIpv4Address(md->GetFrom());
    eventCountMap[senderAddress].value = eventCount;

    /* A mensagem de response é enviada em resposta a uma mensagem anteriormente recebida
    e que está na lista de acks pendentes. Por esse motivo, ela será registrada no log de
    eventos através do método checkForAcksAndLogEvents, ao processar o ack enviado. */

    return false;
}

void ECSProtocol::checkForAcksAndLogEvents(Ptr<MessageData> md){

    Ipv4Address from = utils::convertAddressToIpv4Address(md->GetFrom());
    int i = 0;
    json j;

    int senderEventCount = (md->getPiggyBackedValue("eventCount") != "" ? 
                                stoi(md->getPiggyBackedValue("eventCount")) : 
                                INT_MAX);

    //Enquanto houver acks a serem analisados
    while (md->GetPiggyBackedInfo().find("ack" + to_string(i)) != string::npos){
        
        //Pegando o próximo ack enviado
        string ackUid = md->getPiggyBackedValue("ack" + to_string(i));

        //Verificando a próxima pendência de confirmação
        Ptr<MessageData> pendingAckMessage = pendingAck[from].front()->GetMessageData();

        //Se a confirmação que chegou é referente à que estava pendente
        if (ackUid == to_string(pendingAckMessage->GetUid())){
            
            //Como o recebimento da mensagem foi confirmado, devemos incrementar o contador de eventos
            //O min serve apenas como reforço de segurança
            eventCountMap[from].value = std::min(senderEventCount, eventCountMap[from].value + 1);

            Ptr<EventRecord> e = Create<EventRecord>();
            e->SetJ(eventCountMap[from].value);
            e->SetMessage(pendingAckMessage);

            //Se esta for a última mensagem a ser confirmada e ela for do tipo request
            if (pendingAck[from].size() == 1 && pendingAckMessage->GetCommand() == REQUEST_VALUE) {
                //Então devemos registrar que ela teve uma resposta (a que foi recebida agora)
                e->SetMSent(vector<Ptr<MessageData>>{md});
            }

            //Atualizando json a ser escrito no log de eventos
            j.push_back(e);
            
            //Removendo a pendência de confirmação
            pendingAck[from].erase(pendingAck[from].begin());

        }

        i++;
    }

    //Se houve algum ack analisado
    if (i != 0){
        
        //Gravando no log de eventos
        json fileContent = checkpointHelper->readLog(getEventLogFileName(md->GetFrom())); //Lendo eventos já existentes no log
        
        if (fileContent.empty())
            fileContent = j;
        else
            fileContent.insert(fileContent.end(), j.begin(), j.end()); //Adicionando o evento novo
        
        checkpointHelper->writeLog(getEventLogFileName(md->GetFrom()), fileContent, true);
    }
}

void ECSProtocol::afterMessageReceive(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);

    if (md->GetCommand() == REQUEST_CHECKPOINT_CREATION){
        afterCheckpointRequestReceived(md);
        return;
    }

    if (md->GetCommand() == REQUEST_RESEND){
        afterRequestResendReceive(md);
        return;
    }

}

void ECSProtocol::afterCheckpointRequestReceived(Ptr<MessageData> md){
    
    //Verificando se o nó enviou o checkpoint em virtude de desligamento
    bool shutdown = !md->getPiggyBackedValue("shutdown").empty();
    
    if (shutdown){
        failedNodes.push_back(md->GetFrom());
    }

    //Verificando se foi enviado algum ack junto à requisição de checkpoint
    checkForAcksAndLogEvents(md);

    //Gravando checkpoint enviado
    json checkpointData = json::parse(md->getPiggyBackedValue("checkpointData"));
    checkpointHelper->writeCheckpoint(getCheckpointFileName(md->GetFrom()), checkpointData);

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", CHECKPOINT DO NÓ "
                << md->GetFromAsString() << " FOI CRIADO PELA ECS. ");

    //Fazendo limpeza do log de eventos do nó
    int refEventCount = checkpointData["checkpointStrategy"]["eventCount"].get<int>();

    json eventLog = checkpointHelper->readLog(getEventLogFileName(md->GetFrom()));
    vector<Ptr<EventRecord>> events = eventLog;
    vector<Ptr<EventRecord>> finalEvents = vector<Ptr<EventRecord>>();

    for (size_t i = 0; i < events.size(); i++){
        Ptr<EventRecord> e = events.at(i);
        
        if (e->GetJ() > refEventCount){
            finalEvents.push_back(e);
        }
    }
    
    checkpointHelper->writeLog(getEventLogFileName(md->GetFrom()), finalEvents, true);
}

void ECSProtocol::afterRequestResendReceive(Ptr<MessageData> md){
    
    //Removendo o nó da lista de nós falhos
    failedNodes.erase(std::remove(failedNodes.begin(), failedNodes.end(), md->GetFrom()), failedNodes.end());

    //Obtendo o último checkpoint do nó requisitante
    json checkpointData = checkpointHelper->readLog(getCheckpointFileName(md->GetFrom()));
    
    //Obtendo eventos do nó desde o último checkpoint
    int eventCount = checkpointData["checkpointStrategy"]["eventCount"].get<int>();
    json eventsLog = getEventsAfterIndex(md->GetFrom(), eventCount);

    //Obtendo mensagens que estavam pendentes de confirmação de recebimento pelo nó solicitante
    vector<Ptr<MessageSenderData>> pendingMsgs = pendingAck[utils::convertAddressToIpv4Address(md->GetFrom())];
    json jPendingMsgs = pendingMsgs;

    //Construindo payload
    string piggyBackedInfo = "checkpointData " + checkpointData.dump() 
                                + " events " + eventsLog.dump() 
                                + " pendingAck " + jPendingMsgs.dump();

    //Enviando resposta
    app->send(RESEND_RESPONSE, 0, md->GetFrom(), false, piggyBackedInfo);

}

json ECSProtocol::getEventsAfterIndex(Address node, int refEventIndex){
    NS_LOG_FUNCTION(this);
    
    //Obtendo log de eventos do nó
    json completeEventsLog = checkpointHelper->readLog(getEventLogFileName(node));
    json result = json::array();

    for (const auto& r : completeEventsLog) {
        if (r.contains("j") && r["j"].is_number_integer()) {
            int j = r["j"].get<int>();
            
            if (j > refEventIndex) {
                result.push_back(r);
            }
        }
    }

    return result;
}

bool ECSProtocol::interceptSend(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);

    if (md->GetCommand() == REQUEST_VALUE || md->GetCommand() == RESPONSE_VALUE){

        //Verificando se o destinatário da mensagem está em falha  
        if (std::find(failedNodes.begin(), failedNodes.end(), md->GetTo()) != failedNodes.end()){
            //A mensagem não será enviada (será descartada)
            return true;
        }

        /** Quando a ECS encaminha uma mensagem para um nó x, ela fica esperando um ack
         * para poder registrar que x de fato recebeu a mensagem no log de eventos. */ 

        Ipv4Address sender = utils::convertStringToIpv4Address(md->getPiggyBackedValue("originalSenderIP"));
        Ipv4Address destination = utils::convertAddressToIpv4Address(md->GetTo());

        Ptr<MessageSenderData> d = Create<MessageSenderData>();
        d->SetMessageData(md);
        d->SetSendEvent(eventCountMap[sender].value);

        vector<Ptr<MessageSenderData>> pendingMessages = pendingAck[destination];
        pendingMessages.push_back(d);
        
        pendingAck[destination] = pendingMessages;
    }

    return false;
}

string ECSProtocol::getCheckpointFileName(Address from){
    string nodeAddress = utils::convertAddressToString(utils::convertAddressToIpv4Address(from));
    return nodeAddress + "-checkpoint.json";
}

string ECSProtocol::getEventLogFileName(Address from){
    string nodeAddress = utils::convertAddressToString(utils::convertAddressToIpv4Address(from));
    return nodeAddress + "-event-log.json";
}

// int ECSProtocol::getAcksQuantity(string payload){
//     string sub = "ack";
//     size_t count = 0;
//     size_t pos = payload.find(sub);

//     while (pos != string::npos) {
//         ++count;
//         pos = payload.find(sub, pos + sub.length());
//     }

//     return count;
// }

void ECSProtocol::printData() {
    // NS_LOG_INFO("csn = " << csn
    //             << ", adjacentNodes.size() = " << adjacentNodes.size() 
    //             << ", events.size() = " << events.size()
    //             << ", eventCount = " << eventCount
    //             << ", checkpointId = " << checkpointId
    //             << "\n");
}

void to_json(json& j, const Ptr<EventRecord>& obj) {
    NS_LOG_FUNCTION("to_json(json& j, const Ptr<EventRecord>& obj)");
    
    j = json {
        {"j", obj->j},
        {"m", obj->m},
        {"mSent", obj->mSent},
    };
}

void from_json(const json& j, Ptr<EventRecord>& obj) {
    NS_LOG_FUNCTION("from_json(const json& j, Ptr<EventRecord>& obj)");

    obj = CreateObject<EventRecord>();
    j.at("j").get_to(obj->j);
    j.at("m").get_to(obj->m);
    j.at("mSent").get_to(obj->mSent);
}

void to_json(json& j, const Ptr<MessageSenderData>& obj){
    NS_LOG_FUNCTION("to_json(json& j, const Ptr<MessageSenderData>& obj)");

    j = json {
        {"messageData", obj->messageData},
        {"sendEvent", obj->sendEvent}
    };
}

void from_json(const json& j, Ptr<MessageSenderData>& obj){
    NS_LOG_FUNCTION("from_json(const json& j, Ptr<MessageSenderData>& obj)");

    obj = CreateObject<MessageSenderData>();
    j.at("messageData").get_to(obj->messageData);
    j.at("sendEvent").get_to(obj->sendEvent);
}

} // Namespace ns3
