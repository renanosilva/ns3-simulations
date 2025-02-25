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
    
    NS_LOG_FUNCTION("Fim do método");
    
    return tid;
}

SyncPredefinedTimesCheckpoint::SyncPredefinedTimesCheckpoint(Time timeInterval, string nodeName, Ptr<CheckpointApp> application){
    NS_LOG_FUNCTION(this);

    interval = timeInterval;
    app = application;
    agendamento = EventId();
    checkpointHelper = Create<CheckpointHelper>(nodeName);

    //checkpointHelper = new CheckpointHelper(nodeName);

    NS_LOG_FUNCTION("Fim do método");
}

SyncPredefinedTimesCheckpoint::SyncPredefinedTimesCheckpoint(){
    NS_LOG_FUNCTION(this);
    NS_LOG_FUNCTION("Fim do método");
}

SyncPredefinedTimesCheckpoint::~SyncPredefinedTimesCheckpoint()
{
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(agendamento);
    NS_LOG_FUNCTION("Fim do método");
}

void SyncPredefinedTimesCheckpoint::startCheckpointing() {
    NS_LOG_FUNCTION(this);

    //Agendando criação de checkpoints
    agendamento = Simulator::Schedule(interval,
                                &SyncPredefinedTimesCheckpoint::writeCheckpoint,
                                this);
    
    NS_LOG_FUNCTION("Fim do método");
}

void SyncPredefinedTimesCheckpoint::writeLog() {
    /*if (logData != ""){
        checkpointHelper->writeLog(logData);
    }

    NS_LOG_INFO("\nAos " << Simulator::Now().As(Time::S) << ", log criado por " 
        << checkpointHelper->getCheckpointBasename()) ;

    logData = "";*/
    
}

void SyncPredefinedTimesCheckpoint::writeCheckpoint() {

    NS_LOG_FUNCTION(this);
    
    if (mayCheckpoint()){
        app->beforeCheckpoint();

        checkpointHelper->writeCheckpoint(app);

        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", checkpoint criado por " 
            << checkpointHelper->getCheckpointBasename());

        decreaseCheckpointEnergy();

        app->afterCheckpoint();
        
    } else {
        checkpointHelper->skipCheckpoint();
    }

    scheduleNextCheckpoint();

    NS_LOG_FUNCTION("Fim do método");
}

Time SyncPredefinedTimesCheckpoint::getDelayToNextCheckpoint(){
    NS_LOG_FUNCTION(this);
    
    double now = Simulator::Now().GetSeconds();
    double intervalSec = interval.GetSeconds();
    double mod = std::fmod(now, intervalSec);
    double nextCheckpointing = intervalSec - mod;
    Time delay = Time(to_string(nextCheckpointing) + "s");

    NS_LOG_FUNCTION("Fim do método");
    return delay;
}

void SyncPredefinedTimesCheckpoint::scheduleNextCheckpoint(){
    NS_LOG_FUNCTION(this);

    //Agendando próximo checkpoint
    Time delay = getDelayToNextCheckpoint();

    //Será agendado com um delay calculado para garantir o intervalo de tempo predefinido
    agendamento = Simulator::Schedule(delay,
                                &SyncPredefinedTimesCheckpoint::writeCheckpoint,
                                this);

    NS_LOG_FUNCTION("Fim do método");
}

void SyncPredefinedTimesCheckpoint::startRollbackToLastCheckpoint() {
    NS_LOG_FUNCTION(this);   
    startRollback(checkpointHelper->getLastCheckpointId());
    NS_LOG_FUNCTION("Fim do método");
}

void SyncPredefinedTimesCheckpoint::startRollback(int checkpointId) {
    NS_LOG_FUNCTION(this);

    app->beforeRollback();
    
    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << 
                    checkpointHelper->getCheckpointBasename() << 
                    " iniciando procedimento de rollback.");
    
    //lendo último checkpoint
    json j = checkpointHelper->readCheckpoint(checkpointId);

    //std::cout << j.dump(4) << std::endl;

    //iniciando recuperação das informações presentes no checkpoint
    app->from_json(j);

    app->afterRollback();

    scheduleNextCheckpoint();

    NS_LOG_FUNCTION("Fim do método");
}

void SyncPredefinedTimesCheckpoint::decreaseCheckpointEnergy() {
    NS_LOG_FUNCTION(this);
    
    //BatteryNodeApp* bna = dynamic_cast<BatteryNodeApp*>(app);
    Ptr<BatteryNodeApp> bna = DynamicCast<BatteryNodeApp>(app);
    
    if (bna){
        
        try {
            bna->decreaseCheckpointEnergy();
        } catch (NodeAsleepException& e) {
            //Nada a fazer
        } catch (NodeDepletedException& e) {
            //Nada a fazer
        } 

    }

    NS_LOG_FUNCTION("Fim do método");
}

bool SyncPredefinedTimesCheckpoint::mayCheckpoint(){
    NS_LOG_FUNCTION(this);
    
    //BatteryNodeApp* bna = dynamic_cast<BatteryNodeApp*>(app);
    Ptr<BatteryNodeApp> bna = DynamicCast<BatteryNodeApp>(app);

    if (bna && (bna->isDepleted() || bna->isSleeping())){
        return false;
    }

    NS_LOG_FUNCTION("Fim do método");
    return true;
}

void to_json(json& j, const SyncPredefinedTimesCheckpoint& obj) {
    NS_LOG_FUNCTION("SyncPredefinedTimesCheckpoint::to_json");
    
    to_json(j, static_cast<const CheckpointStrategy&>(obj));
    j["strategy"] = "SyncPredefinedTimesCheckpoint";
    j["interval"] = obj.interval.GetTimeStep();
    j["app.typeid.uid"] = obj.app->GetTypeId().GetUid();

    NS_LOG_FUNCTION("Fim do método");
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

    NS_LOG_FUNCTION("Fim do método");
}

} // Namespace ns3
