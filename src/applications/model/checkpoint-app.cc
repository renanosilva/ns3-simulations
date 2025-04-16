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
#include "ns3/inet-socket-address.h"
#include "ns3/global-sync-clocks-strategy.h"
#include "ns3/log-utils.h"

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
    
    return tid;
}

CheckpointApp::CheckpointApp()
{
    NS_LOG_FUNCTION(this);
}

CheckpointApp::~CheckpointApp()
{
    NS_LOG_FUNCTION(this);
}

void CheckpointApp::configureCheckpointStrategy() {
    NS_LOG_FUNCTION(this);
    
    string property = "simulation.checkpoint-strategy";
    string checkpointStrategyName = configHelper->GetStringProperty(property);

    if (checkpointStrategyName == "GlobalSyncClocksStrategy"){
        
        string intervalProperty = "simulation.checkpoint-interval";
        double checkpointInterval = configHelper->GetDoubleProperty(intervalProperty);

        string timeoutProperty = "simulation.checkpoint-timeout";
        double checkpointTimeout = configHelper->GetDoubleProperty(timeoutProperty);

        checkpointStrategy = Create<GlobalSyncClocksStrategy>(Seconds(checkpointInterval), Seconds(checkpointTimeout), this);
        checkpointStrategy->startCheckpointing();
    
    } else {
        NS_ABORT_MSG("Não foi possível identificar a estratégia de checkpoint de " << getNodeName());
    }
}

Ptr<MessageData> CheckpointApp::send(string command, int d, Address to){
    Ptr<MessageData> md = udpHelper->send(command, d, to);
    utils::logRegularMessageSent(getNodeName(), md);
    return md;
}

Ptr<MessageData> CheckpointApp::send(string command, int d, Ipv4Address ip, uint16_t port){
    Ptr<MessageData> md = udpHelper->send(command, d, ip, port);
    utils::logRegularMessageSent(getNodeName(), md);
    return md;
}

bool CheckpointApp::mayCheckpoint(){
    return true;
}

bool CheckpointApp::mayRemoveCheckpoint(){
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

void CheckpointApp::beforeCheckpointDiscard(){
    
}

void CheckpointApp::afterCheckpointDiscard(){
    
}

void CheckpointApp::beforeRollback(){
    
}

void CheckpointApp::afterRollback(){
    
}

void CheckpointApp::initiateRollback(Address requester, int cpId){

}

void CheckpointApp::decreaseEnergy(double amount) {
    NS_LOG_FUNCTION(this);
    //Por padrão, não faz nada. Nós a bateria devem sobrescrever o método.
}

void CheckpointApp::decreaseCheckpointEnergy(){
    NS_LOG_FUNCTION(this);
    //Por padrão, não faz nada. Nós a bateria devem sobrescrever o método.
}

void CheckpointApp::decreaseRollbackEnergy(){
    NS_LOG_FUNCTION(this);
    //Por padrão, não faz nada. Nós a bateria devem sobrescrever o método.
}

void CheckpointApp::decreaseReadEnergy(){
    NS_LOG_FUNCTION(this);
    //Por padrão, não faz nada. Nós a bateria devem sobrescrever o método.
}

void CheckpointApp::decreaseSendEnergy(){
    NS_LOG_FUNCTION(this);
    //Por padrão, não faz nada. Nós a bateria devem sobrescrever o método.
}

ApplicationType CheckpointApp::getApplicationType(){
    return applicationType;
}

json CheckpointApp::to_json() const {
    NS_LOG_FUNCTION(this);
    
    json j = Application::to_json();
    j["udpHelper"] = *udpHelper;
    j["checkpointStrategy"] = *checkpointStrategy;
    j["nodeName"] = nodeName;
    j["configFilename"] = configFilename;
    
    return j;
}

void CheckpointApp::from_json(const json& j) {
    NS_LOG_FUNCTION(this);

    Application::from_json(j);

    udpHelper = Create<UDPHelper>();

    j.at("udpHelper").get_to(*udpHelper); 
    j.at("checkpointStrategy").get_to(*checkpointStrategy);
    j.at("nodeName").get_to(nodeName);
    j.at("configFilename").get_to(configFilename);
}

string CheckpointApp::getNodeName(){
    NS_LOG_FUNCTION(this);
    return nodeName;
}

} // Namespace ns3
