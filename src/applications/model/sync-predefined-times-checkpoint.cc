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
    static TypeId tid =
        TypeId("ns3::SyncPredefinedTimesCheckpoint")
            .SetParent<CheckpointStrategy>()
            .SetGroupName("Checkpoint")
            .AddAttribute("interval",
                          "Intervalo de tempo no qual serão criados checkpoints.",
                          TimeValue(Seconds(5.0)), //valor inicial
                          MakeTimeAccessor(&SyncPredefinedTimesCheckpoint::interval),
                          MakeTimeChecker());
    return tid;
}

SyncPredefinedTimesCheckpoint::SyncPredefinedTimesCheckpoint(Time timeInterval, string nodeName, Application *application){
    NS_LOG_FUNCTION(this);

    interval = timeInterval;
    checkpointHelper = new CheckpointHelper(nodeName);
    app = application;
}

SyncPredefinedTimesCheckpoint::SyncPredefinedTimesCheckpoint(){

}

SyncPredefinedTimesCheckpoint::~SyncPredefinedTimesCheckpoint()
{
    NS_LOG_FUNCTION(this);
}

void SyncPredefinedTimesCheckpoint::startCheckpointing() {
    //Agendando criação de checkpoints
    Simulator::Schedule(interval,
                                &SyncPredefinedTimesCheckpoint::writeCheckpoint,
                                this);
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

    if (mayCheckpoint()){
        checkpointHelper->writeCheckpoint(app);

        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", checkpoint criado por " 
            << checkpointHelper->getCheckpointBasename());

        decreaseCheckpointEnergy();
    } else {
        checkpointHelper->skipCheckpoint();
    }

    scheduleNextCheckpoint();
}

Time SyncPredefinedTimesCheckpoint::getDelayToNextCheckpoint(){
    double now = Simulator::Now().GetSeconds();
    double intervalSec = interval.GetSeconds();
    double mod = std::fmod(now, intervalSec);
    double nextCheckpointing = intervalSec - mod;
    Time delay = Time(to_string(nextCheckpointing) + "s");

    return delay;
}

void SyncPredefinedTimesCheckpoint::scheduleNextCheckpoint(){
    //Agendando próximo checkpoint
    Time delay = getDelayToNextCheckpoint();

    //Será agendado com um delay calculado para garantir o intervalo de tempo predefinido
    Simulator::Schedule(delay,
                                &SyncPredefinedTimesCheckpoint::writeCheckpoint,
                                this);
}

void SyncPredefinedTimesCheckpoint::startRollback() {
    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << 
                    checkpointHelper->getCheckpointBasename() << 
                    " iniciando procedimento de rollback.");
    
    //lendo último checkpoint
    json j = checkpointHelper->readLastCheckpoint();

    //iniciando recuperação das informações presentes no checkpoint
    app->from_json(j);

    scheduleNextCheckpoint();
}

void SyncPredefinedTimesCheckpoint::decreaseCheckpointEnergy() {
    BatteryNodeApp* bna = dynamic_cast<BatteryNodeApp*>(app);
    
    if (bna){
        
        try {
            bna->decreaseCheckpointEnergy();
        } catch (NodeAsleepException& e) {
            //Nada a fazer
        } catch (NodeDepletedException& e) {
            //Nada a fazer
        } 

    }
}

bool SyncPredefinedTimesCheckpoint::mayCheckpoint(){
    BatteryNodeApp* bna = dynamic_cast<BatteryNodeApp*>(app);
    
    if (bna && (bna->isDepleted() || bna->isSleeping())){
        return false;
    }

    return true;
}

void to_json(json& j, const SyncPredefinedTimesCheckpoint& obj) {
    to_json(j, static_cast<const CheckpointStrategy&>(obj));
    j["strategy"] = "SyncPredefinedTimesCheckpoint";
    j["interval"] = obj.interval.GetTimeStep();
    j["app.typeid.uid"] = obj.app->GetTypeId().GetUid();
}

void from_json(const json& j, SyncPredefinedTimesCheckpoint& obj) {
    from_json(j, static_cast<CheckpointStrategy&>(obj));

    double v = 0.;
    j.at("interval").get_to(v);
    Time t = Time(v);
    
    obj.interval = t;
    
    //o atributo app é redefinido através da própria aplicação
    //sendo assim, não é necessário redefini-lo aqui
}

} // Namespace ns3
