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

#include "checkpoint-strategy.h"
using namespace std;

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CheckpointStrategy");

NS_OBJECT_ENSURE_REGISTERED(CheckpointStrategy);

TypeId
CheckpointStrategy::GetTypeId()
{
    NS_LOG_FUNCTION(CheckpointStrategy::GetTypeId());
    static TypeId tid =
        TypeId("ns3::CheckpointStrategy")
            .AddConstructor<CheckpointStrategy>()
            .SetParent<Object>()
            .SetGroupName("Sensor");

    NS_LOG_FUNCTION("Fim do método");
    return tid;
}

CheckpointStrategy::CheckpointStrategy(){
    NS_LOG_FUNCTION(this);
    NS_LOG_FUNCTION("Fim do método");
}

CheckpointStrategy::~CheckpointStrategy()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_FUNCTION("Fim do método");
}

void CheckpointStrategy::startCheckpointing(){
    
}

void CheckpointStrategy::stopCheckpointing(){
    
}

void CheckpointStrategy::writeLog(){
    
}

void CheckpointStrategy::writeCheckpoint(){
    
}

void CheckpointStrategy::discardLastCheckpoint(){
    
}

void CheckpointStrategy::startRollbackToLastCheckpoint(){
    
}

void CheckpointStrategy::startRollback(int checkpointId){
    
}

void CheckpointStrategy::confirmLastCheckpoint(){
    
}

int CheckpointStrategy::getLastCheckpointId(){
    NS_LOG_FUNCTION(this);
    return checkpointHelper->getLastCheckpointId();
}

void CheckpointStrategy::setApp(Ptr<CheckpointApp> application){
    NS_LOG_FUNCTION(this);
    app = application;
    NS_LOG_FUNCTION("Fim do método");
}

void to_json(json& j, const CheckpointStrategy& obj) {
    NS_LOG_FUNCTION("CheckpointStrategy::to_json");
    
    j = json{
        {"checkpointHelper", *obj.checkpointHelper}, 
        {"logData", obj.logData}
    };
    
    NS_LOG_FUNCTION("Fim do método");
}

void from_json(const json& j, CheckpointStrategy& obj) {
    NS_LOG_FUNCTION("CheckpointStrategy::from_json");
    
    j.at("checkpointHelper").get_to(*obj.checkpointHelper);
    j.at("logData").get_to(obj.logData);

    NS_LOG_FUNCTION("Fim do método");
}

void CheckpointStrategy::setLogData(string data){
    NS_LOG_FUNCTION(this);
    logData = data;
}

void CheckpointStrategy::addLogData(string data){
    NS_LOG_FUNCTION(this);
    logData += data;
}

string CheckpointStrategy::getLogData(){
    NS_LOG_FUNCTION(this);
    return logData;
}

} // Namespace ns3
