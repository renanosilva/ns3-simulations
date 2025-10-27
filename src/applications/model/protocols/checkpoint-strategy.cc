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
#include "ns3/log-utils.h"

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

    return tid;
}

CheckpointStrategy::CheckpointStrategy(){
    NS_LOG_FUNCTION(this);
}

CheckpointStrategy::~CheckpointStrategy()
{
    NS_LOG_FUNCTION(this);
}

void CheckpointStrategy::DisposeReferences(){
    
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

void CheckpointStrategy::rollbackToLastCheckpoint(){
    
}

void CheckpointStrategy::beforeBatteryDischarge(){
    
}

bool CheckpointStrategy::rollback(int checkpointId){
    return false;
}

void CheckpointStrategy::rollback(Address requester, int checkpointId, string piggyBackedInfo){
    
}

void CheckpointStrategy::confirmLastCheckpoint(){
    
}

void CheckpointStrategy::addAddress(vector<Address> &v, Address a){
    NS_LOG_FUNCTION(this);
    
    auto it = find(v.begin(), v.end(), a);

    if (it == v.end()) {
        //Endereço não foi encontrado
        //Adiciona endereço ao vetor de endereços
        v.push_back(a);
    }
}

void CheckpointStrategy::addDependentAddress(Address a){
    NS_LOG_FUNCTION(this);
    
    auto it = find(dependentAddresses.begin(), dependentAddresses.end(), a);

    if (it == dependentAddresses.end()) {
        //Endereço não foi encontrado
        //Adiciona endereço ao vetor de endereços
        dependentAddresses.push_back(a);
    }
}

void CheckpointStrategy::removeAddress(vector<Address> &v, Address a){
    // Função de comparação para encontrar o endereço exato
    auto it = std::remove_if(v.begin(), v.end(),
                             [a](const Address &addr)
                             {
                                 return InetSocketAddress::ConvertFrom(addr).GetIpv4() ==
                                        InetSocketAddress::ConvertFrom(a).GetIpv4();
                             });

    // Remove efetivamente o elemento do vetor
    if (it != v.end()){
        v.erase(it, v.end());
    }
}

bool CheckpointStrategy::isCheckpointInProgress(){
    return checkpointInProgress;
}

bool CheckpointStrategy::isRollbackInProgress(){
    return rollbackInProgress;
}

bool CheckpointStrategy::interceptRead(Ptr<MessageData> md){
    return false;
}

void CheckpointStrategy::afterMessageReceive(Ptr<MessageData> md){
    
}

bool CheckpointStrategy::interceptSend(Ptr<MessageData> md){
    return false;
}

vector<Address> CheckpointStrategy::getDependentAddresses(){
    return dependentAddresses;
}

int CheckpointStrategy::getLastCheckpointId(){
    NS_LOG_FUNCTION(this);
    return checkpointHelper->getLastCheckpointId();
}

void CheckpointStrategy::confirmCheckpointCreation(bool confirm) {

}

void CheckpointStrategy::printData(){
    NS_LOG_INFO("");
}

json CheckpointStrategy::to_json() {
    NS_LOG_FUNCTION("CheckpointStrategy::to_json");
    
    json j = json{
        {"dependentAddresses", dependentAddresses}
    };
    
    return j;
}

void CheckpointStrategy::from_json(const json& j) {
    NS_LOG_FUNCTION("CheckpointStrategy::from_json");
    
    j.at("dependentAddresses").get_to(dependentAddresses); 
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
