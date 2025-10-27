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

#include "no-rollback-2-with-ecs.h"
#include "ns3/server-node-app.h"
#include "ns3/simulator.h"
#include "ns3/node-depleted-exception.h"
#include "ns3/node-asleep-exception.h"
#include "ns3/log-utils.h"
#include "ns3/application-type.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NoRollback2WithECS");
NS_OBJECT_ENSURE_REGISTERED(NoRollback2WithECS);

TypeId
NoRollback2WithECS::GetTypeId()
{
    NS_LOG_FUNCTION(NoRollback2WithECS::GetTypeId());

    static TypeId tid =
        TypeId("ns3::NoRollback2WithECS")
            .SetParent<CheckpointStrategy>()
            .SetGroupName("Checkpoint")
            .AddConstructor<NoRollback2WithECS>()
            .AddAttribute("interval",
                          "Intervalo de tempo no qual serão criados checkpoints.",
                          TimeValue(Seconds(5.0)), //valor inicial
                          MakeTimeAccessor(&NoRollback2WithECS::interval),
                          MakeTimeChecker());
    
    return tid;
}

NoRollback2WithECS::NoRollback2WithECS(Time timeInterval, Ptr<CheckpointApp> application){
    NS_LOG_FUNCTION(this);

    app = StaticCast<ECSCheckpointApp>(application);
    interval = timeInterval;
    eventCount = -1;
    creationScheduling = EventId();
    checkpointHelper = Create<CheckpointHelper>(application->getNodeName());
    rollbackInProgress = false;
}

NoRollback2WithECS::NoRollback2WithECS(){
    NS_LOG_FUNCTION(this);
    eventCount = -1;
    rollbackInProgress = false;
}

NoRollback2WithECS::~NoRollback2WithECS()
{
    NS_LOG_FUNCTION(this);
}

void NoRollback2WithECS::DisposeReferences(){
    stopCheckpointing();
}

void NoRollback2WithECS::startCheckpointing() {
    NS_LOG_FUNCTION(this);
    scheduleNextCheckpoint();
}

void NoRollback2WithECS::stopCheckpointing() {
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(creationScheduling);
}

void NoRollback2WithECS::requestCheckpointCreation() {
    NS_LOG_FUNCTION(this);

    if (!app->mayCheckpoint() || rollbackInProgress){
        scheduleNextCheckpoint();
        return;
    }

    try {
        if (!app->hasEnoughEnergy(2.0*app->getSendPacketConsumption())){
            //Não adianta criar um checkpoint agora pois outro será criado em breve, quando o nó se descarregar
            scheduleNextCheckpoint();
            return;
        }

        json j = app->to_json();

        app->sendToECS(REQUEST_CHECKPOINT_CREATION, 0, "checkpointData " + j.dump());
        
        //Descontando a energia de forma dobrada, pois o payload é grande
        app->decreaseSendEnergy();

        scheduleNextCheckpoint();

    } catch (NodeAsleepException& e) {
        //Operações interrompidas... Nó irá entrar em modo sleep. Nada mais a fazer.
    } catch (NodeDepletedException& e) {
        //Operações interrompidas... Nó irá entrar em modo depleted. Nada mais a fazer.
    } 
}

Time NoRollback2WithECS::getDelayToNextCheckpoint(){
    NS_LOG_FUNCTION(this);
    
    return interval;
}

void NoRollback2WithECS::scheduleNextCheckpoint(){
    NS_LOG_FUNCTION(this);

    //Agendando próximo checkpoint
    Time delay = getDelayToNextCheckpoint();

    //Será agendado com um delay calculado para garantir o intervalo de tempo predefinido
    creationScheduling = Simulator::Schedule(delay,
                                &NoRollback2WithECS::requestCheckpointCreation,
                                this);
}

void NoRollback2WithECS::rollbackToLastCheckpoint() {
    NS_LOG_FUNCTION(this);   
    
    try {
        rollbackInProgress = true;

        app->beforeRollback();

        app->StartApplication();

        //Solicitando à ECS que reenvie o último checkpoint e as mensagens registradas antes da falha
        app->sendToECS(REQUEST_RESEND, 0);

    } catch (NodeAsleepException& e) {
        NS_LOG_LOGIC("Tarefa incompleta por estar em modo SLEEP.");
        return;
    } catch (NodeDepletedException& e) {
        NS_LOG_LOGIC("Tarefa incompleta por estar em modo DEPLETED.");
        return;
    }
}

bool NoRollback2WithECS::interceptRead(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this << md);

    try {
        if (!app->isAlive()
            || ((md->GetCommand() == REQUEST_VALUE 
                || md->GetCommand() == RESPONSE_VALUE) 
                && rollbackInProgress)){
            
            return true;
        }

        if (md->GetCommand() == REQUEST_VALUE || md->GetCommand() == RESPONSE_VALUE){
            
            //O recebimento de uma mensagem de request ou response gera um novo evento
            ++eventCount;

            //A mensagem recebida é inserida na lista de confirmações pendentes
            pendingAcksToSend.push_back(md);

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

void NoRollback2WithECS::afterMessageReceive(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);

    //Se o comando contém o termo RESEND_RESPONSE
    if (md->GetCommand() == RESEND_RESPONSE){
        
        //Reprocessando as mensagens
        afterResendMessageReceive(md);
        return;
    }
}

void NoRollback2WithECS::afterResendMessageReceive(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);

    try {
         //Calculando desconto de energia a ser realizado
        uint32_t msgsSize = md->GetSize();
        uint32_t multiple = msgsSize/700;
        
        // for (uint32_t i = 0; i < multiple; i++){
        //     //Como a mensagem é grande, o desconto de energia é maior
        //     app->decreaseReadEnergy();
        // }

        //Obtendo payload
        string piggyBackedInfo = md->GetPiggyBackedInfo();

        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << app->getNodeName() 
                            << " iniciando procedimento de ROLLBACK.");

        //Parando a aplicação
        app->StopApplication();

        //Obtendo o checkpoint enviado
        string checkpointData = getResentData(piggyBackedInfo, "checkpointData");
        json jsonCheckpointData = json::parse(checkpointData);

        //Recupera o estado da aplicação presente no último checkpoint
        app->from_json(jsonCheckpointData);

        //Reiniciando a aplicação
        app->StartApplication();

        //Obtendo os eventos que devem passar por replay
        string eventsString = getResentData(piggyBackedInfo, "events");
        json eventsJson = json::parse(eventsString);
        vector<Ptr<EventRecord>> events = eventsJson;
        
        /* Fazendo o replay das mensagens reenviadas. Isso é necessário para que o nó volte para o 
        mesmo estado em que se encontrava antes da falha. */  
        for (size_t i = 0; i < events.size(); i++){
            replayEvent(events.at(i));
        }

        //Obtendo as mensagens que estavam pendentes de ack
        string pendingAckString = getResentData(piggyBackedInfo, "pendingAck");
        json pendingAckJson = json::parse(pendingAckString);
        vector<Ptr<MessageSenderData>> pendingAckMsgs = pendingAckJson.get<vector<Ptr<MessageSenderData>>>();
        
        for (size_t i = 0; i < pendingAckMsgs.size(); i++){
            Ptr<MessageSenderData> data = pendingAckMsgs.at(0);

            //Recriando evento referente à mensagem que foi recebida e estava pendente de ack
            Ptr<EventRecord> e = Create<EventRecord>();
            e->SetJ(eventCount + 1);
            e->SetMessage(data->GetMessageData());

            //Fazendo replay do evento para registrá-lo
            replayEvent(e);

            //Registrando que a mensagem ainda está pendente de envio de ack
            pendingAcksToSend.push_back(data->GetMessageData());
        }
    
        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << app->getNodeName() 
                << " concluiu o procedimento de rollback e reprocessou todos os eventos.");

        app->decreaseRollbackEnergy();

        rollbackInProgress = false;

        NS_LOG_INFO("\nDepois do rollback...");
        app->printNodeData();

    } catch (NodeAsleepException& e) {
        NS_LOG_LOGIC("Tarefa incompleta por estar em modo SLEEP.");
        return;
    } catch (NodeDepletedException& e) {
        NS_LOG_LOGIC("Tarefa incompleta por estar em modo DEPLETED.");
        return;
    }
}

void NoRollback2WithECS::replayEvent(Ptr<EventRecord> e){
    NS_LOG_FUNCTION(this);

    ++eventCount;

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) 
                << ", " << app->getNodeName() 
                << " realizando replay das mensagens presentes no evento " 
                << e->GetJ() << "...");

    Ptr<MessageData> msgReceived = e->GetMessage();

    //Se houve alguma mensagem recebida no evento
    if (msgReceived != nullptr && msgReceived->GetCommand() != ""){
        
        //Faz o processamento do recebimento
        app->replayReceive(msgReceived, false, false);
    }

    //Se houve alguma mensagem enviada no evento
    if (!e->GetMSent().empty()){
        
        for (Ptr<MessageData> md : e->GetMSent()){
            app->send(md->GetCommand(), md->GetData(), md->GetTo(), true, md->GetPiggyBackedInfo());
        }
    }
}

string NoRollback2WithECS::getResentData(const string& piggyBackedData, const string& key){
    // 1. Localiza a chave no texto
    size_t keyPos = piggyBackedData.find(key);
    if (keyPos == std::string::npos) {
        return ""; // chave não encontrada
    }

    // 2. Pula o nome da chave + um espaço
    size_t valueStart = piggyBackedData.find_first_not_of(" ", keyPos + key.size());
    if (valueStart == std::string::npos) {
        return "";
    }

    char openingChar = piggyBackedData[valueStart];
    char closingChar;

    if (openingChar == '{') closingChar = '}';
    else if (openingChar == '[') closingChar = ']';
    else {
        // valor não é um objeto nem array — pegar até próximo espaço
        size_t valueEnd = piggyBackedData.find(' ', valueStart);
        if (valueEnd == std::string::npos) valueEnd = piggyBackedData.size();
        return piggyBackedData.substr(valueStart, valueEnd - valueStart);
    }

    // 3. Encontrar o fechamento correspondente
    int depth = 0;
    size_t valueEnd = valueStart;
    for (; valueEnd < piggyBackedData.size(); ++valueEnd) {
        if (piggyBackedData[valueEnd] == openingChar) depth++;
        else if (piggyBackedData[valueEnd] == closingChar) depth--;

        if (depth == 0) break;
    }

    if (depth != 0) {
        throw std::runtime_error("Delimitadores não balanceados para a chave: " + key);
    }

    // 4. Extrair a substring correspondente
    return piggyBackedData.substr(valueStart, valueEnd - valueStart + 1);
}

bool NoRollback2WithECS::interceptSend(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);

    if (!app->isAlive()
        || ((md->GetCommand() == REQUEST_VALUE 
            || md->GetCommand() == RESPONSE_VALUE) 
            && rollbackInProgress)){
            
            return true;
    }

    if (md->GetCommand() == REQUEST_VALUE){

        //O envio de uma mensagem de request gera um novo evento
        ++eventCount;
        
    }

    if (md->GetCommand() == REQUEST_VALUE || md->GetCommand() == RESPONSE_VALUE){
        
        // O envio de uma mensagem de response não gera evento novo

        //Adicionando o contador de eventos para que a ECS tome conhecimento.
        md->addPiggyBackedInfo("eventCount " + to_string(eventCount));

        addAcksToMessage(md);

        return false;
    }

    if (md->GetCommand() == REQUEST_CHECKPOINT_CREATION){

        addAcksToMessage(md);
        return false;

    }

    return false;
}

void NoRollback2WithECS::addAcksToMessage(Ptr<MessageData> md){
    
    //Adicionando acks (caso haja)
    for (uint i = 0; i < pendingAcksToSend.size(); i++){
        Ptr<MessageData> m = pendingAcksToSend.at(i);
        md->addPiggyBackedInfo("ack" + to_string(i) + " " + to_string(m->GetUid()));
    }

    pendingAcksToSend.clear();
}

void NoRollback2WithECS::beforeBatteryDischarge(){

    if (!isRollbackInProgress()){
        //Criando e enviando checkpoint para a ECS com aviso de desligamento 
        json j = app->to_json();
        app->sendToECS(REQUEST_CHECKPOINT_CREATION, 0, "shutdown true checkpointData " + j.dump());
    }
        
}

void NoRollback2WithECS::printData() {
    NS_LOG_INFO("eventCount = " << eventCount
                << ", checkpointId = " << checkpointId
                << "\n");
}

json NoRollback2WithECS::to_json() {
    NS_LOG_FUNCTION("NoRollback2WithECS::to_json");
    
    json j = CheckpointStrategy::to_json();
    j["eventCount"] = eventCount;
    
    return j;
}

void NoRollback2WithECS::from_json(const json& j) {
    NS_LOG_FUNCTION("NoRollback2WithECS::from_json");
    
    CheckpointStrategy::from_json(j);
    j.at("eventCount").get_to(eventCount);
}

} // Namespace ns3
