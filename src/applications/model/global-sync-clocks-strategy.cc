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

#include "global-sync-clocks-strategy.h"
#include "ns3/battery-node-app.h"
#include "ns3/simulator.h"
#include "ns3/node-depleted-exception.h"
#include "ns3/node-asleep-exception.h"
#include "ns3/log-utils.h"
#include "ns3/application-type.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("GlobalSyncClocksStrategy");

NS_OBJECT_ENSURE_REGISTERED(GlobalSyncClocksStrategy);

TypeId
GlobalSyncClocksStrategy::GetTypeId()
{
    NS_LOG_FUNCTION(GlobalSyncClocksStrategy::GetTypeId());

    static TypeId tid =
        TypeId("ns3::GlobalSyncClocksStrategy")
            .SetParent<CheckpointStrategy>()
            .SetGroupName("Checkpoint")
            .AddConstructor<GlobalSyncClocksStrategy>()
            .AddAttribute("interval",
                          "Intervalo de tempo no qual serão criados checkpoints.",
                          TimeValue(Seconds(5.0)), //valor inicial
                          MakeTimeAccessor(&GlobalSyncClocksStrategy::interval),
                          MakeTimeChecker());
    
    return tid;
}

GlobalSyncClocksStrategy::GlobalSyncClocksStrategy(Time timeInterval, Time tout, Ptr<CheckpointApp> application){
    NS_LOG_FUNCTION(this);

    interval = timeInterval;
    timeout = tout;
    app = application;
    creationScheduling = EventId();
    discardScheduling = EventId();
    checkpointHelper = Create<CheckpointHelper>(application->getNodeName());
    rollbackInProgress = false;
    checkpointInProgress = false;
    dependentAddresses.clear();
    pendingRollbackAddresses.clear();
    pendingCheckpointAddresses.clear();
}

GlobalSyncClocksStrategy::GlobalSyncClocksStrategy(){
    NS_LOG_FUNCTION(this);
    rollbackInProgress = false;
    checkpointInProgress = false;
    dependentAddresses.clear();
    pendingRollbackAddresses.clear();
    pendingCheckpointAddresses.clear();
}

GlobalSyncClocksStrategy::~GlobalSyncClocksStrategy()
{
    NS_LOG_FUNCTION(this);
    
    dependentAddresses.clear();
    pendingRollbackAddresses.clear();
    pendingCheckpointAddresses.clear();

    stopCheckpointing();
}

//VERIFICAR COMO FORÇAR A EXCLUSÃO DE UM PTR

void GlobalSyncClocksStrategy::startCheckpointing() {
    NS_LOG_FUNCTION(this);
    scheduleNextCheckpoint();
}

void GlobalSyncClocksStrategy::stopCheckpointing() {
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(creationScheduling);
    Simulator::Cancel(discardScheduling);
}

void GlobalSyncClocksStrategy::writeCheckpoint() {
    NS_LOG_FUNCTION(this);
    
    try {
        
        if (app->mayCheckpoint()){
            int time = Simulator::Now().GetSeconds();

            checkpointInProgress = true;
            pendingCheckpointAddresses = vector<Address>(dependentAddresses);

            NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << app->getNodeName() 
                                            << " entrou no modo de bloqueio de comunicação para"
                                            << " iniciar procedimento de criação de CHECKPOINT.");

            checkpointHelper->writeCheckpoint(app, time);

            NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", checkpoint criado por " 
                << checkpointHelper->getCheckpointBasename() << ".");

            app->decreaseCheckpointEnergy();

            //Se não houver nós dependentes, confirma o checkpoint
            //Caso contrário, fica aguardando o recebimento das confirmações dos outros nós
            if (pendingCheckpointAddresses.size() == 0){
                confirmCheckpointCreation(true);
            }

            if (pendingCheckpointAddresses.size() > 0 && app->getApplicationType() == SERVER){
                notifyNodesAboutCheckpointConcluded();
            }

            //Abordagem pessimista: agenda a remoção do checkpoint caso o timeout seja atingido
            //Caso o checkpoint seja confirmado, esse agendamento é cancelado
            scheduleLastCheckpointDiscard();
        
        } else {
            scheduleNextCheckpoint();
        }

    } catch (NodeAsleepException& e) {
        //Operações interrompidas... Nó irá entrar em modo sleep. Nada mais a fazer.
    } catch (NodeDepletedException& e) {
        //Operações interrompidas... Nó irá entrar em modo depleted. Nada mais a fazer.
    } 

    NS_LOG_FUNCTION("Fim do método");
}

void GlobalSyncClocksStrategy::discardLastCheckpoint() {
    NS_LOG_FUNCTION(this);
    
    try {

        if (app->mayRemoveCheckpoint()){
            checkpointHelper->removeCheckpoint(checkpointHelper->getLastCheckpointId());

            NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", checkpoint DESCARTADO por " 
                << checkpointHelper->getCheckpointBasename() << ".");

            confirmCheckpointCreation(false);
        }

        //Agenda o próximo checkpoint
        scheduleNextCheckpoint();
    
    } catch (NodeAsleepException& e) {
        //Operações interrompidas... Nó irá entrar em modo sleep. Nada mais a fazer.
    } catch (NodeDepletedException& e) {
        //Operações interrompidas... Nó irá entrar em modo depleted. Nada mais a fazer.
    } 

    NS_LOG_FUNCTION("Fim do método");
}

void GlobalSyncClocksStrategy::confirmLastCheckpoint() {
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", checkpoint CONFIRMADO por " 
            << checkpointHelper->getCheckpointBasename() << ".");
    
    //Cancela agendamento da remoção do checkpoint
    Simulator::Cancel(discardScheduling);

    //Agenda o próximo checkpoint
    scheduleNextCheckpoint();
}

Time GlobalSyncClocksStrategy::getDelayToNextCheckpoint(){
    NS_LOG_FUNCTION(this);
    
    double now = Simulator::Now().GetSeconds();
    double intervalSec = interval.GetSeconds();
    double mod = std::fmod(now, intervalSec);
    double nextCheckpointing = intervalSec - mod;
    Time delay = Time(to_string(nextCheckpointing) + "s");

    return delay;
}

void GlobalSyncClocksStrategy::scheduleNextCheckpoint(){
    NS_LOG_FUNCTION(this);

    NS_LOG_LOGIC("\n" << checkpointHelper->getCheckpointBasename() << " agendando próximo checkpoint\n");

    //Agendando próximo checkpoint
    Time delay = getDelayToNextCheckpoint();

    //Será agendado com um delay calculado para garantir o intervalo de tempo predefinido
    creationScheduling = Simulator::Schedule(delay,
                                &GlobalSyncClocksStrategy::writeCheckpoint,
                                this);
    NS_LOG_FUNCTION("Fim do método");
}

void GlobalSyncClocksStrategy::scheduleLastCheckpointDiscard(){
    NS_LOG_FUNCTION(this);
    
    //Será agendado com um delay calculado para garantir o intervalo de tempo predefinido
    discardScheduling = Simulator::Schedule(timeout,
                                &GlobalSyncClocksStrategy::discardLastCheckpoint,
                                this);
}

void GlobalSyncClocksStrategy::rollbackToLastCheckpoint() {
    NS_LOG_FUNCTION(this);   
    
    checkpointId = getLastCheckpointId();
    rollback(checkpointId);
}

void GlobalSyncClocksStrategy::rollback(int checkpointId) {
    NS_LOG_FUNCTION(this);
    
    app->beforeRollback();

    rollbackInProgress = true;

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << app->getNodeName() 
                                    << " entrou no modo de bloqueio de comunicação para iniciar procedimento de ROLLBACK.");
    
    
    //lendo checkpoint
    NS_LOG_LOGIC("Checkpoint ID: " << checkpointId);
    json j = checkpointHelper->readCheckpoint(checkpointId);

    // std::cout << "\nDados do JSON lido:\n" << j.dump(4) << std::endl;

    //iniciando recuperação das informações presentes no checkpoint
    app->from_json(j);

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << app->getNodeName() 
            << " concluiu o procedimento de rollback.");
    
    app->decreaseRollbackEnergy();

    app->afterRollback();

    //Copiando os endereços para o vetor de rollbacks pendentes
    pendingRollbackAddresses = vector<Address>(dependentAddresses);

    if (pendingRollbackAddresses.size() > 0){
       
        //Enviando mensagem de rollback para os outros nós envolvidos
        notifyNodesAboutRollback();
        
    } else {
       
        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << app->getNodeName() 
                            << " saiu do modo de bloqueio de comunicação.");
            
        rollbackInProgress = false;
    }
}

void GlobalSyncClocksStrategy::rollback(Address requester, int cpId) {
    rollbackStarterIp = InetSocketAddress::ConvertFrom(requester).GetIpv4();
    checkpointId = cpId;
    rollback(checkpointId);
}

bool GlobalSyncClocksStrategy::interceptRead(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this << md);
    
    try {
        if (app->getApplicationType() == SERVER){
            if (checkpointInProgress){
                return interceptServerReadInCheckpointMode(md);
            }
            
            if (rollbackInProgress){
                return interceptServerReadInRollbackMode(md);
            }

            if (md->GetCommand() == REQUEST_TO_START_ROLLBACK_COMMAND){
                utils::logMessageReceived(app->getNodeName(), md);
                app->decreaseReadEnergy();

                app->initiateRollback(md->GetFrom(), md->GetData());
                return true;
            }
        }

        if (app->getApplicationType() == CLIENT){
            if (checkpointInProgress){
                return interceptClientReadInCheckpointMode(md);
            }
            
            if (rollbackInProgress){
                return interceptClientReadInRollbackMode(md);
            }

            if (md->GetCommand() == REQUEST_TO_START_ROLLBACK_COMMAND){
                utils::logMessageReceived(app->getNodeName(), md);
                app->initiateRollback(md->GetFrom(), md->GetData());
                return true;
            }
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

bool GlobalSyncClocksStrategy::interceptServerReadInRollbackMode(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);

    if (rollbackInProgress){
        
        if (md->GetCommand() != ROLLBACK_FINISHED_COMMAND){
            //Ignora pacotes até a conclusão do rollback dos outros nós
            return true;
        }

        utils::logMessageReceived(app->getNodeName(), md);

        if (md->GetCommand() == ROLLBACK_FINISHED_COMMAND){
            removeAddress(pendingRollbackAddresses, md->GetFrom());

            //Só sai do rollback caso todos os nós tenham terminado seus rollbacks
            if (pendingRollbackAddresses.empty()){

                NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << app->getNodeName() 
                                << " saiu do modo de bloqueio de comunicação.");
                
                rollbackInProgress = false;
            }

            app->decreaseReadEnergy();
            return true;
        }

    }

    return false;
}

bool GlobalSyncClocksStrategy::interceptServerReadInCheckpointMode(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);
    
    if (checkpointInProgress){

        if (md->GetCommand() == REQUEST_VALUE){
            //Ignora pacotes até a conclusão do checkpoint dos outros nós
            return true;
        }

        utils::logMessageReceived(app->getNodeName(), md);

        if (md->GetCommand() == CHECKPOINT_FINISHED_COMMAND){
            removeAddress(pendingCheckpointAddresses, md->GetFrom());

            //Só termina o checkpointing caso todos os nós tenham terminado seus checkpoints
            if (pendingCheckpointAddresses.empty()){
                confirmCheckpointCreation(true);
            }

            app->decreaseReadEnergy();
            return true;
        }

        //Também é possível receber requisições de rollback enquanto se está aguardando 
        //confirmação de checkpoint. Nesse caso, cancela-se o checkpoint e faz-se o rollback
        if (md->GetCommand() == REQUEST_TO_START_ROLLBACK_COMMAND){
            discardLastCheckpoint();
            app->initiateRollback(md->GetFrom(), md->GetData());
            return true;
        }
    }

    return false;
}

bool GlobalSyncClocksStrategy::interceptClientReadInRollbackMode(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);

    if (md->GetCommand() == RESPONSE_VALUE){
        //Ignora pacotes até a conclusão do rollback dos outros nós
        return true;
    }

    utils::logMessageReceived(app->getNodeName(), md);

    //Se receber uma notificação de término de rollback de outro nó
    //OU se receber uma requisição para voltar ao estado no qual já se encontra
    if (md->GetCommand() == ROLLBACK_FINISHED_COMMAND || 
        (md->GetCommand() == REQUEST_TO_START_ROLLBACK_COMMAND &&
        addressVectorContain(pendingRollbackAddresses, md->GetFrom()) &&
        checkpointId == md->GetData())){
        
        //Avisando o nó emissor que o rollback foi concluído
        Ptr<MessageData> mSent = app->send(ROLLBACK_FINISHED_COMMAND, 0, md->GetFrom());
        // utils::logRegularMessageSent(app->getNodeName(), mSent);

        removeAddress(pendingRollbackAddresses, md->GetFrom());

        //Só sai do rollback caso todos os nós tenham terminado seus rollbacks
        if (pendingRollbackAddresses.empty()){
            notifyNodesAboutRollbackConcluded();
        }

        return true;
    }

    return false;
}

bool GlobalSyncClocksStrategy::interceptClientReadInCheckpointMode(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this);
    
    if (checkpointInProgress){

        if (md->GetCommand() == RESPONSE_VALUE){
            //Ignora pacotes até a conclusão do rollback dos outros nós
            return true;
        }

        utils::logMessageReceived(app->getNodeName(), md);

        if (md->GetCommand() == CHECKPOINT_FINISHED_COMMAND){
            removeAddress(pendingCheckpointAddresses, md->GetFrom());

            //Só termina o checkpointing caso todos os nós tenham terminado seus checkpoints
            if (pendingCheckpointAddresses.empty()){
                notifyNodesAboutCheckpointConcluded();
                confirmCheckpointCreation(true);
            }

            return true;
        }

        /* Também é possível receber requisições de rollback enquanto se está aguardando 
        confirmação de checkpoint. Nesse caso, cancela-se o checkpoint e faz-se o rollback */
        if (md->GetCommand() == REQUEST_TO_START_ROLLBACK_COMMAND){
            discardLastCheckpoint();
            app->initiateRollback(md->GetFrom(), md->GetData());
            return true;
        }
    }

    return false;
}

bool GlobalSyncClocksStrategy::interceptSend(Ptr<MessageData> md){
    if ((rollbackInProgress || checkpointInProgress) && 
        (md->GetCommand() == REQUEST_VALUE || md->GetCommand() == RESPONSE_VALUE)){
        
            return true;
    }
    
    if (md->GetCommand() == REQUEST_VALUE || md->GetCommand() == RESPONSE_VALUE){
        addDependentAddress(md->GetTo());
    }

    return false;
}

void GlobalSyncClocksStrategy::notifyNodesAboutCheckpointConcluded(){
    NS_LOG_FUNCTION(this);

    for (Address a : dependentAddresses){
        
        // Enviando o pacote para o destino
        Ptr<MessageData> md = app->send(CHECKPOINT_FINISHED_COMMAND, getLastCheckpointId(), a);
        // utils::logRegularMessageSent(app->getNodeName(), md);
    }
    
    app->decreaseSendEnergy();
}

void GlobalSyncClocksStrategy::notifyNodesAboutRollback(){
    NS_LOG_FUNCTION(this);

    if (app->getApplicationType() == CLIENT){
        Ptr<ClientNodeApp> client = DynamicCast<ClientNodeApp>(app);
        removeAddress(pendingRollbackAddresses, InetSocketAddress(rollbackStarterIp, client->getPeerPort()));
    }

    // Enviando notificação para os nós com os quais houve comunicação
    if (pendingRollbackAddresses.size() > 0){

        for (Address a : pendingRollbackAddresses){
            
            // Enviando o pacote para o destino
            Ptr<MessageData> md = app->send(REQUEST_TO_START_ROLLBACK_COMMAND, checkpointId, a);
            // utils::logRegularMessageSent(app->getNodeName(), md);
        }
    
        app->decreaseSendEnergy();
    }

    if (pendingRollbackAddresses.size() == 0 && app->getApplicationType() == CLIENT){
        //Se não existem outros nós a fazerem rollback, então o rollback está concluído 
        notifyNodesAboutRollbackConcluded();
    }
}

void GlobalSyncClocksStrategy::notifyNodesAboutRollbackConcluded(){
    NS_LOG_FUNCTION(this);

    Ptr<ClientNodeApp> client = DynamicCast<ClientNodeApp>(app);

    if (client){
        Ptr<MessageData> md = app->send(ROLLBACK_FINISHED_COMMAND, 0, rollbackStarterIp, client->getPeerPort());
        // utils::logRegularMessageSent(getNodeName(), md);

        rollbackInProgress = false;
        rollbackStarterIp = Ipv4Address::GetAny();

        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << app->getNodeName() 
        << " saiu do modo de bloqueio de comunicação.");

    }
}

void GlobalSyncClocksStrategy::confirmCheckpointCreation(bool confirm) {
    NS_LOG_FUNCTION(this);
    
    checkpointInProgress = false;

    //Removendo endereços com os quais este nó se comunicou no ciclo anterior
    dependentAddresses.clear();

    if (confirm){
        confirmLastCheckpoint();
    }
    
    app->decreaseCheckpointEnergy();

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << app->getNodeName() 
        << " saiu do modo de bloqueio de comunicação.");
}

void to_json(json& j, const GlobalSyncClocksStrategy& obj) {
    NS_LOG_FUNCTION("GlobalSyncClocksStrategy::to_json");
    
    to_json(j, static_cast<const CheckpointStrategy&>(obj));
}

void from_json(const json& j, GlobalSyncClocksStrategy& obj) {
    NS_LOG_FUNCTION("GlobalSyncClocksStrategy::from_json");
    
    from_json(j, static_cast<CheckpointStrategy&>(obj));
}

} // Namespace ns3
