/*
 * Copyright (c) 2008 INRIA
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
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "node-app-helper.h"

#include "ns3/udp-echo-client.h"
#include "ns3/ecs-node-app.h"
#include "ns3/server-node-app.h"
#include "ns3/client-node-app.h"
#include "ns3/ecs-responsor-node-app.h"
#include "ns3/ecs-requestor-node-app.h"
#include "ns3/uinteger.h"

namespace ns3
{

ECSNodeAppHelper::ECSNodeAppHelper(uint16_t ecsPort, string nodeName, string configFilename, int nodesQuantity)
    : ApplicationHelper(ECSNodeApp::GetTypeId()) {
    
    SetAttribute("Port", UintegerValue(ecsPort));
    SetAttribute("NodeName", StringValue(nodeName));
    SetAttribute("ConfigFilename", StringValue(configFilename));
    SetAttribute("TotalNodesQuantity", IntegerValue(nodesQuantity));

}

ResponsorNodeAppHelper::ResponsorNodeAppHelper(uint16_t serverPort, string nodeName, Ipv4Address& ecsAddress, uint16_t ecsPort, 
                                                string configFilename, int nodesQuantity)
    : ApplicationHelper(ECSResponsorNodeApp::GetTypeId()) {
    
    SetAttribute("Port", UintegerValue(serverPort));
    SetAttribute("NodeName", StringValue(nodeName));
    SetAttribute("ECSAddress", StringValue(utils::convertAddressToString(ecsAddress)));
    SetAttribute("ECSPort", UintegerValue(ecsPort));
    SetAttribute("ConfigFilename", StringValue(configFilename));
    SetAttribute("TotalNodesQuantity", IntegerValue(nodesQuantity));

}

RequestorNodeAppHelper::RequestorNodeAppHelper(vector<Ipv4Address>& serverAddresses, uint16_t serverPort, Ipv4Address& ecsAddress, 
    uint16_t ecsPort, string nodeName, string configFilename, int nodesQuantity)
    : ApplicationHelper(ECSRequestorNodeApp::GetTypeId()) {

    SetAttribute("RemoteAddresses", StringValue(utils::convertAddressesToString(serverAddresses)));
    SetAttribute("RemotePort", UintegerValue(serverPort));
    SetAttribute("Port", UintegerValue(serverPort));
    SetAttribute("ECSAddress", StringValue(utils::convertAddressToString(ecsAddress)));
    SetAttribute("ECSPort", UintegerValue(ecsPort));
    SetAttribute("NodeName", StringValue(nodeName));
    SetAttribute("ConfigFilename", StringValue(configFilename));
    SetAttribute("TotalNodesQuantity", IntegerValue(nodesQuantity));

}

ServerNodeAppHelper::ServerNodeAppHelper(uint16_t port, string nodeName, string configFilename, int nodesQuantity)
    : ApplicationHelper(ServerNodeApp::GetTypeId()) {
    
    SetAttribute("Port", UintegerValue(port));
    SetAttribute("NodeName", StringValue(nodeName));
    SetAttribute("ConfigFilename", StringValue(configFilename));
    SetAttribute("TotalNodesQuantity", IntegerValue(nodesQuantity));

}

ClientNodeAppHelper::ClientNodeAppHelper(vector<Ipv4Address>& addresses, uint16_t port, string nodeName, string configFilename, int nodesQuantity)
    : ApplicationHelper(ClientNodeApp::GetTypeId()) {
    
    SetAttribute("RemoteAddresses", StringValue(utils::convertAddressesToString(addresses)));
    SetAttribute("RemotePort", UintegerValue(port));
    SetAttribute("NodeName", StringValue(nodeName));
    SetAttribute("ConfigFilename", StringValue(configFilename));
    SetAttribute("TotalNodesQuantity", IntegerValue(nodesQuantity));

}

} // namespace ns3
