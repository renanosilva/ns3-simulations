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

#include "decentralized-recovery-protocol.h"
#include "ns3/server-node-app.h"
#include "ns3/simulator.h"
#include "ns3/node-depleted-exception.h"
#include "ns3/node-asleep-exception.h"
#include "ns3/log-utils.h"
#include "ns3/application-type.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DecentralizedRecoveryProtocol");

NS_OBJECT_ENSURE_REGISTERED(DecentralizedRecoveryProtocol);

TypeId
DecentralizedRecoveryProtocol::GetTypeId()
{
    NS_LOG_FUNCTION(DecentralizedRecoveryProtocol::GetTypeId());

    static TypeId tid =
        TypeId("ns3::DecentralizedRecoveryProtocol")
            .SetParent<CheckpointStrategy>()
            .SetGroupName("Checkpoint")
            .AddConstructor<DecentralizedRecoveryProtocol>()
            .AddAttribute("interval",
                          "Intervalo de tempo no qual serão criados checkpoints.",
                          TimeValue(Seconds(5.0)), //valor inicial
                          MakeTimeAccessor(&DecentralizedRecoveryProtocol::interval),
                          MakeTimeChecker());
    
    return tid;
}

DecentralizedRecoveryProtocol::DecentralizedRecoveryProtocol(Time timeInterval, Ptr<CheckpointApp> application){
    NS_LOG_FUNCTION(this);

    interval = timeInterval;
    csn = -1;
    activeCheckpoint = -1;
    app = application;
    creationScheduling = EventId();
    checkpointHelper = Create<CheckpointHelper>(application->getNodeName());
    propList.clear();
}

DecentralizedRecoveryProtocol::DecentralizedRecoveryProtocol(){
    NS_LOG_FUNCTION(this);
    csn = -1;
    activeCheckpoint = -1;
    propList.clear();
}

DecentralizedRecoveryProtocol::~DecentralizedRecoveryProtocol()
{
    NS_LOG_FUNCTION(this);
    
    propList.clear();

    stopCheckpointing();
}

void DecentralizedRecoveryProtocol::startCheckpointing() {
    NS_LOG_FUNCTION(this);

    /* Logo ao iniciar, cria um checkpoint.
    É importante, pois, em caso de efeito dominó, pode ser que seja 
    necessário voltar ao estado inicial do sistema. */

    //Agendando a criação do checkpoint para logo após o início da aplicação
    Time delay = Time("0.000000000000000000000001s");
    creationScheduling = Simulator::Schedule(delay, &DecentralizedRecoveryProtocol::writeCheckpoint, this);
    
}

void DecentralizedRecoveryProtocol::stopCheckpointing() {
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(creationScheduling);
}

void DecentralizedRecoveryProtocol::writeCheckpoint() {
    NS_LOG_FUNCTION(this);

    if (!app->mayCheckpoint()){
        scheduleNextCheckpoint();
        return;
    }

    try {
        checkpointHelper->writeCheckpoint(app, ++csn);
        activeCheckpoint = csn;

        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", CHECKPOINT " << csn << " CRIADO POR " 
            << checkpointHelper->getCheckpointBasename() << ".");

        propList.clear();

        app->decreaseCheckpointEnergy();

        scheduleNextCheckpoint();

    } catch (NodeAsleepException& e) {
        //Operações interrompidas... Nó irá entrar em modo sleep. Nada mais a fazer.
    } catch (NodeDepletedException& e) {
        //Operações interrompidas... Nó irá entrar em modo depleted. Nada mais a fazer.
    } 
}

Time DecentralizedRecoveryProtocol::getDelayToNextCheckpoint(){
    NS_LOG_FUNCTION(this);
    
    // double now = Simulator::Now().GetSeconds();
    // double intervalSec = interval.GetSeconds();
    // double mod = std::fmod(now, intervalSec);
    // double nextCheckpointing = intervalSec - mod;
    // Time delay = Time(to_string(nextCheckpointing) + "s");
    
    // return delay;

    return interval;
}

void DecentralizedRecoveryProtocol::scheduleNextCheckpoint(){
    NS_LOG_FUNCTION(this);

    NS_LOG_LOGIC("\n" << checkpointHelper->getCheckpointBasename() << " agendando próximo checkpoint\n");

    //Agendando próximo checkpoint
    Time delay = getDelayToNextCheckpoint();

    //Será agendado com um delay calculado para garantir o intervalo de tempo predefinido
    creationScheduling = Simulator::Schedule(delay,
                                &DecentralizedRecoveryProtocol::writeCheckpoint,
                                this);
}

void DecentralizedRecoveryProtocol::rollbackToLastCheckpoint() {
    NS_LOG_FUNCTION(this);   
    
    checkpointId = getLastCheckpointId();
    
    //Se o rollback não for bem-sucedido
    while (!rollback(checkpointId)){
        /*
        * Isso significa que há um problema com o checkpoint.
        * Nesse caso, tentar rollback para o checkpoint imediatamente anterior.
        */

        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << app->getNodeName() 
                                    << " falhou ao tentar fazer um rollback. Tentando novamente"
                                    << " com um checkpoint anterior...");
        
        checkpointId = checkpointHelper->getPreviousCheckpointId(checkpointId);
    }

    //Se o nó não tiver descarregado durante o processo
    if (!app->isDepleted() && !app->isSleeping()){
        //Enviando mensagem de requisição de rollback para os nós dependentes
        notifyAllNodesAboutRollback();
    }
}

void DecentralizedRecoveryProtocol::rollback(Address requester, int cpId, string piggyBackedInfo) {
    NS_LOG_FUNCTION(this);

    checkpointId = cpId;

    //Se o rollback não for bem-sucedido
    while (!rollback(checkpointId)){
        /*
        * Isso significa que há um problema com o checkpoint.
        * Nesse caso, tentar rollback para o checkpoint imediatamente anterior.
        */

        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << app->getNodeName() 
                                    << " falhou ao tentar fazer um rollback. Tentando novamente"
                                    << " com um checkpoint anterior...");
        
        checkpointId = checkpointHelper->getPreviousCheckpointId(checkpointId);
    }

    //Se o nó não tiver descarregado durante o processo
    if (!app->isDepleted() && !app->isSleeping()){
            istringstream iss(piggyBackedInfo);
            string command;
            int activeCheckpointId;
            iss >> command >> activeCheckpointId;

        notifyNodesAboutRollback(requester, activeCheckpointId);
    }
}

bool DecentralizedRecoveryProtocol::rollback(int checkpointId) {
    NS_LOG_FUNCTION(this);
    
    app->beforeRollback();

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << app->getNodeName() 
                                    << " iniciando procedimento de ROLLBACK para o checkpoint " << checkpointId << ".");
    
    json j;

    try {

        //lendo checkpoint
        j = checkpointHelper->readCheckpoint(checkpointId);

    } catch (const json::parse_error& e){
        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << app->getNodeName() 
                                    << " FALHOU ao tentar realizar ROLLBACK. Checkpoint inválido.");
        return false;
    }
    
    /* Verificando se existem dependências inválidas.
    Uma dependência é inválida se ela tiver sido armazenada com o
    activeCheckpoint com id -1. Nesse caso, não se sabe qual era
    o checkpoint ativo do nó com o qual existia dependência. */
    if (j.at("checkpointStrategy").at("propList").dump().find("-1") != string::npos){
        return false;
    }

    //iniciando recuperação das informações presentes no checkpoint
    app->from_json(j);
    
    /* A numeração de sequência de checkpoint é atualizada para que não haja
    sobrescrita de checkpoints */
    csn = checkpointHelper->getLastCheckpointId();

    //O checkpoint ativo é o checkpoint para o qual foi feito rollback
    activeCheckpoint = checkpointId;
    
    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << app->getNodeName() 
            << " concluiu o procedimento de rollback.");
    
    app->decreaseRollbackEnergy();

    app->afterRollback();

    NS_LOG_INFO("\nDepois do rollback...");
    app->printNodeData();

    return true;
}

void DecentralizedRecoveryProtocol::notifyAllNodesAboutRollback(){
    NS_LOG_FUNCTION(this);

    // Enviando notificação para os nós com os quais houve comunicação
    if (propList.size() > 0){

        for (DRPNodeInfo i : propList){
            
            // Enviando o pacote para o destino
            Ptr<MessageData> md = app->send(REQUEST_TO_START_ROLLBACK_COMMAND, i.GetActiveCheckpoint(), i.GetAddress());
        }
    
    }
}

void DecentralizedRecoveryProtocol::notifyNodesAboutRollback(Address requester, int requesterActiveCheckpoint){
    NS_LOG_FUNCTION(this);

    // Enviando notificação para os nós com os quais houve comunicação
    if (propList.size() > 0){

        for (DRPNodeInfo i : propList){
            
            //Não envia requisição de rollback caso o solicitante já esteja no checkpoint esperado
            if (i.GetAddress() == requester && i.GetActiveCheckpoint() == requesterActiveCheckpoint)
                continue;
            
            // Enviando o pacote para o destino
            Ptr<MessageData> md = app->send(REQUEST_TO_START_ROLLBACK_COMMAND, i.GetActiveCheckpoint(), i.GetAddress());
        }
    
    }
}

bool DecentralizedRecoveryProtocol::interceptRead(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this << md);
    
    try {

        if (md->GetCommand() == REQUEST_VALUE || md->GetCommand() == RESPONSE_VALUE){
            
            //Obtendo o ID do checkpoint ativo do emissor da mensagem
            string piggyBackedInfo = md->GetPiggyBackedInfo();

            istringstream iss(piggyBackedInfo);
            string command;
            int activeCheckpointId;
            iss >> command >> activeCheckpointId;

            DRPNodeInfo i = DRPNodeInfo(md->GetFrom(), activeCheckpointId);

            //adicionando informação à propList
            addToPropList(i);

        }
        
        if (md->GetCommand() == REQUEST_TO_START_ROLLBACK_COMMAND){
            utils::logMessageReceived(app->getNodeName(), md);
            app->decreaseReadEnergy();

            app->initiateRollback(md->GetFrom(), md->GetData(), md->GetPiggyBackedInfo());

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

bool DecentralizedRecoveryProtocol::interceptSend(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);

    if (md->GetCommand() == REQUEST_VALUE || md->GetCommand() == RESPONSE_VALUE){
        md->SetPiggyBackedInfo("activeCheckpoint " + to_string(checkpointHelper->getLastCheckpointId()));
     
        DRPNodeInfo* result = findNodeInfoByAddress(md->GetTo());

        if (result == nullptr){
            
            DRPNodeInfo i = DRPNodeInfo();
            i.SetAddress(md->GetTo());
            i.SetActiveCheckpoint(-1); //não se sabe qual o checkpoint ativo do nó, então inicialmente atribui-se -1

            addToPropList(i);
        }
            
    }

    if (md->GetCommand() == REQUEST_TO_START_ROLLBACK_COMMAND){
        md->SetPiggyBackedInfo("activeCheckpoint " + to_string(activeCheckpoint));
    }

    return false;
}

void DecentralizedRecoveryProtocol::addToPropList(DRPNodeInfo i){
    NS_LOG_FUNCTION(this);
    
    DRPNodeInfo* result = findNodeInfoByAddress(i.GetAddress());

    if (result == nullptr) {
        
        //Endereço não foi encontrado
        //Adiciona novo elemento ao vetor
        propList.push_back(i);
        editActiveCheckpointPropList();
    
    } else {

        if (result->GetActiveCheckpoint() == -1){
            
            /* Nesse caso, não se sabia previamente o checkpoint ativo do elemento.
            O checkpoint ativo será, então, o que foi informado agora. */
            result->SetActiveCheckpoint(i.GetActiveCheckpoint());
            editActiveCheckpointPropList();

        } else {
            
            if (i.GetActiveCheckpoint() < result->GetActiveCheckpoint()){
                //Modifica o checkpoint ativo do elemento para o mais antigo (pois será necessário fazer rollback para o mais antigo)
                result->SetActiveCheckpoint(i.GetActiveCheckpoint());
                editActiveCheckpointPropList();
            }
        }

    }
}

void DecentralizedRecoveryProtocol::editActiveCheckpointPropList(){
    int active = checkpointHelper->getLastCheckpointId();
    
    json j = checkpointHelper->readCheckpoint(active);
    j.at("checkpointStrategy")["propList"] = propList;

    checkpointHelper->editCheckpoint(j, active);
}

DRPNodeInfo* DecentralizedRecoveryProtocol::findNodeInfoByAddress(const Address& addr) {
    auto it = find_if(propList.begin(), propList.end(), [&addr](const DRPNodeInfo& n) {
        return n.GetAddress() == addr;
    });

    if (it != propList.end()) {
        return &(*it); // Retorna ponteiro para o DRPNodeInfo encontrado
    }

    return nullptr; // Não encontrado
}

void DecentralizedRecoveryProtocol::printData() {
    NS_LOG_INFO("csn = " << csn << ", propList.size() = " << propList.size() << "\n");
}

void to_json(json& j, const DRPNodeInfo& obj) {
    j = json{
        {"activeCheckpoint", obj.activeCheckpoint}, 
        {"address", obj.address}
    };
}

void from_json(const json& j, DRPNodeInfo& obj) {
    j.at("activeCheckpoint").get_to(obj.activeCheckpoint);
    j.at("address").get_to(obj.address);
}

json DecentralizedRecoveryProtocol::to_json() {
    NS_LOG_FUNCTION("DecentralizedRecoveryProtocol::to_json");
    
    json j = CheckpointStrategy::to_json();
    j["csn"] = csn;
    j["propList"] = propList;

    return j;
}

void DecentralizedRecoveryProtocol::from_json(const json& j) {
    NS_LOG_FUNCTION("DecentralizedRecoveryProtocol::from_json");
    
    CheckpointStrategy::from_json(j);
    j.at("csn").get_to(csn);
    j.at("propList").get_to(propList);
}

} // Namespace ns3
