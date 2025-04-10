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

#include "sync-predefined-times-checkpoint.h"
#include "ns3/battery-node-app.h"
#include "ns3/simulator.h"
#include "ns3/node-depleted-exception.h"
#include "ns3/node-asleep-exception.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SyncPredefinedTimesCheckpoint");

NS_OBJECT_ENSURE_REGISTERED(SyncPredefinedTimesCheckpoint);

TypeId
SyncPredefinedTimesCheckpoint::GetTypeId()
{
    NS_LOG_FUNCTION(SyncPredefinedTimesCheckpoint::GetTypeId());

    static TypeId tid =
        TypeId("ns3::SyncPredefinedTimesCheckpoint")
            .SetParent<CheckpointStrategy>()
            .SetGroupName("Checkpoint")
            .AddConstructor<SyncPredefinedTimesCheckpoint>()
            .AddAttribute("interval",
                          "Intervalo de tempo no qual serão criados checkpoints.",
                          TimeValue(Seconds(5.0)), //valor inicial
                          MakeTimeAccessor(&SyncPredefinedTimesCheckpoint::interval),
                          MakeTimeChecker());
    
    return tid;
}

SyncPredefinedTimesCheckpoint::SyncPredefinedTimesCheckpoint(Time timeInterval, Time tout, Ptr<CheckpointApp> application){
    NS_LOG_FUNCTION(this);

    interval = timeInterval;
    timeout = tout;
    app = application;
    creationScheduling = EventId();
    discardScheduling = EventId();
    checkpointHelper = Create<CheckpointHelper>(application->getNodeName());
}

SyncPredefinedTimesCheckpoint::SyncPredefinedTimesCheckpoint(){
    NS_LOG_FUNCTION(this);
}

SyncPredefinedTimesCheckpoint::~SyncPredefinedTimesCheckpoint()
{
    NS_LOG_FUNCTION(this);
    stopCheckpointing();
}

//VERIFICAR COMO FORÇAR A EXCLUSÃO DE UM PTR

void SyncPredefinedTimesCheckpoint::startCheckpointing() {
    NS_LOG_FUNCTION(this);
    scheduleNextCheckpoint();
}

void SyncPredefinedTimesCheckpoint::stopCheckpointing() {
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(creationScheduling);
    Simulator::Cancel(discardScheduling);
}

void SyncPredefinedTimesCheckpoint::writeCheckpoint() {
    NS_LOG_FUNCTION(this);
    
    try {
        
        if (app->mayCheckpoint()){
            int time = Simulator::Now().GetSeconds();

            app->beforePartialCheckpoint();

            checkpointHelper->writeCheckpoint(app, time);

            NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", checkpoint criado por " 
                << checkpointHelper->getCheckpointBasename() << ".");

            app->afterPartialCheckpoint();

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

void SyncPredefinedTimesCheckpoint::discardLastCheckpoint() {
    NS_LOG_FUNCTION(this);
    
    try {

        if (app->mayRemoveCheckpoint()){
            app->beforeCheckpointDiscard();

            checkpointHelper->removeCheckpoint(checkpointHelper->getLastCheckpointId());

            NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", checkpoint DESCARTADO por " 
                << checkpointHelper->getCheckpointBasename() << ".");

            app->afterCheckpointDiscard();
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

void SyncPredefinedTimesCheckpoint::confirmLastCheckpoint() {
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", checkpoint CONFIRMADO por " 
            << checkpointHelper->getCheckpointBasename() << ".");
    
    //Cancela agendamento da remoção do checkpoint
    Simulator::Cancel(discardScheduling);

    //Agenda o próximo checkpoint
    scheduleNextCheckpoint();
}

Time SyncPredefinedTimesCheckpoint::getDelayToNextCheckpoint(){
    NS_LOG_FUNCTION(this);
    
    double now = Simulator::Now().GetSeconds();
    double intervalSec = interval.GetSeconds();
    double mod = std::fmod(now, intervalSec);
    double nextCheckpointing = intervalSec - mod;
    Time delay = Time(to_string(nextCheckpointing) + "s");

    return delay;
}

void SyncPredefinedTimesCheckpoint::scheduleNextCheckpoint(){
    NS_LOG_FUNCTION(this);

    NS_LOG_LOGIC("\n" << checkpointHelper->getCheckpointBasename() << " agendando próximo checkpoint\n");

    //Agendando próximo checkpoint
    Time delay = getDelayToNextCheckpoint();

    //Será agendado com um delay calculado para garantir o intervalo de tempo predefinido
    creationScheduling = Simulator::Schedule(delay,
                                &SyncPredefinedTimesCheckpoint::writeCheckpoint,
                                this);
    NS_LOG_FUNCTION("Fim do método");
}

void SyncPredefinedTimesCheckpoint::scheduleLastCheckpointDiscard(){
    NS_LOG_FUNCTION(this);
    
    //Será agendado com um delay calculado para garantir o intervalo de tempo predefinido
    discardScheduling = Simulator::Schedule(timeout,
                                &SyncPredefinedTimesCheckpoint::discardLastCheckpoint,
                                this);
}

void SyncPredefinedTimesCheckpoint::startRollbackToLastCheckpoint() {
    NS_LOG_FUNCTION(this);   
    startRollback(checkpointHelper->getLastCheckpointId());
}

void SyncPredefinedTimesCheckpoint::startRollback(int checkpointId) {
    NS_LOG_FUNCTION(this);

    app->beforeRollback();
    
    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << 
                    checkpointHelper->getCheckpointBasename() << 
                    " iniciando procedimento de rollback.");
    
    //lendo checkpoint
    NS_LOG_LOGIC("Checkpoint ID: " << checkpointId);
    json j = checkpointHelper->readCheckpoint(checkpointId);

    //std::cout << "\nDados do JSON lido:\n" << j.dump(4) << std::endl;

    //iniciando recuperação das informações presentes no checkpoint
    app->from_json(j);

    app->afterRollback();

}

void to_json(json& j, const SyncPredefinedTimesCheckpoint& obj) {
    NS_LOG_FUNCTION("SyncPredefinedTimesCheckpoint::to_json");
    
    to_json(j, static_cast<const CheckpointStrategy&>(obj));
    j["strategy"] = "SyncPredefinedTimesCheckpoint";
    j["interval"] = obj.interval.GetTimeStep();
    j["app.typeid.uid"] = obj.app->GetTypeId().GetUid();
}

void from_json(const json& j, SyncPredefinedTimesCheckpoint& obj) {
    NS_LOG_FUNCTION("SyncPredefinedTimesCheckpoint::from_json");
    
    from_json(j, static_cast<CheckpointStrategy&>(obj));

    double v = 0.;
    j.at("interval").get_to(v);
    Time t = Time(v);
    
    obj.interval = t;
    
    //o atributo app é redefinido através da própria aplicação
    //sendo assim, não é necessário redefini-lo aqui
}

} // Namespace ns3
