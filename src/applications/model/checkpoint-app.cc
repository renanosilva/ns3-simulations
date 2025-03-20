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

#include "checkpoint-app.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CheckpointApp");
NS_OBJECT_ENSURE_REGISTERED(CheckpointApp);

TypeId
CheckpointApp::GetTypeId()
{
    NS_LOG_FUNCTION("CheckpointApp::GetTypeId()");

    static TypeId tid =
        TypeId("ns3::CheckpointApp")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<CheckpointApp>();
    
    NS_LOG_FUNCTION("Fim do método");
    return tid;
}

CheckpointApp::CheckpointApp()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_FUNCTION("Fim do método");
}

CheckpointApp::~CheckpointApp()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_FUNCTION("Fim do método");
    //delete checkpointStrategy;
}

void CheckpointApp::configureCheckpointStrategy() {
    
}

bool CheckpointApp::mayCheckpoint(){
    return true;
}

void
CheckpointApp::StartApplication()
{
    NS_LOG_FUNCTION(this);
}

void
CheckpointApp::StopApplication()
{
    NS_LOG_FUNCTION(this);
}

void CheckpointApp::beforeCheckpoint(){
    
}

void CheckpointApp::afterCheckpoint(){
    
}

void CheckpointApp::beforeRollback(){
    
}

void CheckpointApp::afterRollback(){
    
}

json CheckpointApp::to_json() const {
    NS_LOG_FUNCTION(this);
    
    json j = Application::to_json();
    j["nodeName"] = nodeName;
    
    NS_LOG_FUNCTION("Fim do método");
    return j;
}

void CheckpointApp::from_json(const json& j) {
    NS_LOG_FUNCTION(this);

    Application::from_json(j);
    j.at("nodeName").get_to(nodeName); 

    NS_LOG_FUNCTION("Fim do método");
}

string CheckpointApp::getNodeName(){
    NS_LOG_FUNCTION(this);
    return nodeName;
}

} // Namespace ns3
