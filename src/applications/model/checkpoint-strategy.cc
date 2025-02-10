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
    static TypeId tid =
        TypeId("ns3::CheckpointStrategy")
            .AddConstructor<CheckpointStrategy>()
            .SetParent<Object>()
            .SetGroupName("Sensor");
    return tid;
}

CheckpointStrategy::CheckpointStrategy(){
    NS_LOG_FUNCTION(this);
}

CheckpointStrategy::~CheckpointStrategy()
{
    NS_LOG_FUNCTION(this);
}

void CheckpointStrategy::startCheckpointing(){
    
}

void CheckpointStrategy::writeLog(){
    
}

void CheckpointStrategy::writeCheckpoint(){
    
}

void CheckpointStrategy::startRollback(){
    
}

void CheckpointStrategy::setApp(Application *application){
    app = application;
}

void to_json(json& j, const CheckpointStrategy& obj) {
    j = json{
        {"checkpointHelper", *obj.checkpointHelper}, 
        {"logData", obj.logData}
    };
}

void from_json(const json& j, CheckpointStrategy& obj) {
    j.at("checkpointHelper").get_to(*obj.checkpointHelper);
    j.at("logData").get_to(obj.logData);
}

void CheckpointStrategy::setLogData(string data){
    logData = data;
}

void CheckpointStrategy::addLogData(string data){
    logData += data;
}

string CheckpointStrategy::getLogData(){
    return logData;
}

} // Namespace ns3
