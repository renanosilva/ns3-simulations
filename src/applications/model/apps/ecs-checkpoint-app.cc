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

#include "ecs-checkpoint-app.h"
#include "ns3/inet-socket-address.h"
#include "ns3/global-sync-clocks-strategy.h"
#include "ns3/decentralized-recovery-protocol.h"
#include "ns3/efficient-assync-recovery-protocol.h"
#include "ns3/no-rollback-1.h"
#include "ns3/no-rollback-2.h"
#include "ns3/no-rollback-2-with-ecs.h"
#include "ns3/ecs-protocol.h"
#include "ns3/no-rollback-2-optimized.h"
#include "ns3/log-utils.h"
#include "ns3/node-depleted-exception.h"
#include "ns3/node-asleep-exception.h"
#include "ns3/utils.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ECSCheckpointApp");
NS_OBJECT_ENSURE_REGISTERED(ECSCheckpointApp);

void ECSCheckpointApp::configureCheckpointStrategy() {
    string property = "simulation.checkpoint-strategy";
    string checkpointStrategyName = configHelper->GetStringProperty(property);

    string roleProperty = "nodes." + getNodeName() + ".role" ;
    string role = configHelper->GetStringProperty(roleProperty);

    if (checkpointStrategyName == "NoRollback2WithECS") {
        
        if (role == "ECS"){
            checkpointStrategy = Create<ECSProtocol>(this);
        } else {
            
            string intervalProperty = "nodes." + getNodeName() + ".checkpoint-interval";
            double checkpointInterval = configHelper->GetDoubleProperty(intervalProperty, -1);

            checkpointStrategy = Create<NoRollback2WithECS>(Seconds(checkpointInterval), this);
            checkpointStrategy->startCheckpointing();
        }

    } else {
        NS_ABORT_MSG("Não foi possível identificar a estratégia de checkpoint de " << getNodeName());
    }
}

Ptr<MessageData> ECSCheckpointApp::sendViaECS(string command, int d, Address to, bool replay, string piggyBackedInfo){
    
    InetSocketAddress inetAddr = InetSocketAddress::ConvertFrom(to);
    Ipv4Address finalDestinationIP = inetAddr.GetIpv4();
    uint16_t finalDestinationPort = inetAddr.GetPort();
    
    piggyBackedInfo = piggyBackedInfo != "" ? " " : "";
    piggyBackedInfo.append("finalDestinationIP " + utils::convertIpv4AddressToString(finalDestinationIP) + " ");
    piggyBackedInfo.append("finalDestinationPort " + to_string(finalDestinationPort));
    
    Ptr<MessageData> md = udpHelper->send(command, d, ecsAddress, ecsPort, replay, piggyBackedInfo);
    
    if (md != nullptr){
        utils::logRegularMessageSent(getNodeName(), md, replay);
        
        if (!replay)
            decreaseSendEnergy();
    }

    return md;
}

Ptr<MessageData> ECSCheckpointApp::sendViaECS(string command, int d, Ipv4Address ip, uint16_t port, bool replay, string piggyBackedInfo){
    InetSocketAddress finalDestination = InetSocketAddress(ip, port);
    return sendViaECS(command, d, finalDestination, replay, piggyBackedInfo);
}

Ptr<MessageData> ECSCheckpointApp::sendToECS(string command, int d, string piggyBackedInfo){
    return send(command, d, ecsAddress, ecsPort, false, piggyBackedInfo);
}

// void ECSCheckpointApp::decreaseEnergy(double amount) {
//     NS_LOG_FUNCTION(this);
    
//     if (battery != nullptr){
//         NS_ASSERT_MSG(
//             (currentMode == EnergyMode::SLEEP && amount == sleepEnergyConsumption) ||
//             (currentMode == EnergyMode::NORMAL), 
//                     "Operação inválida! Bateria está em modo sleep e não pode realizar operações.");
    
//         battery->decrementEnergy(amount);

//         NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", energia consumida por " << getNodeName() << 
//                     ": " << amount << ". Energia restante: " << to_string(battery->getRemainingEnergy()) << ".");

//         if (battery->getBatteryPercentage() <= sleepModePercentage && mayCheckMode && isAlive()){
         
//             mayCheckMode = false;
            
//             //Nó irá se descarregar. Dando a oportunidade ao protocolo de realizar algum processamento antes disso.
//             checkpointStrategy->beforeBatteryDischarge();

//             mayCheckMode = true;
//         }
        
//         if (mayCheckMode)
//             checkModeChange();
//     }
// }

void ECSCheckpointApp::SetECSAddress(string address)
{
    NS_LOG_FUNCTION(this);
    
    istringstream iss(address);
    string token;

    getline(iss, token);
    ecsAddress = Ipv4Address(token.c_str());
}

string ECSCheckpointApp::GetECSAddress() const {
    NS_LOG_FUNCTION(this);
    
    return GetAddressAsString(ecsAddress);
}

string ECSCheckpointApp::GetAddressAsString(Address a) const {
    NS_LOG_FUNCTION(this);
    
    ostringstream oss;
    oss << a;

    return oss.str();
}

json ECSCheckpointApp::to_json() const {
    NS_LOG_FUNCTION(this);
    
    json j = CheckpointApp::to_json();

    return j;
}

void ECSCheckpointApp::from_json(const json& j) {
    NS_LOG_FUNCTION(this);

    CheckpointApp::from_json(j);
}

} // Namespace ns3
