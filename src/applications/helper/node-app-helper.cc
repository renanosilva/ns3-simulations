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
#include "ns3/server-node-app.h"
#include "ns3/client-node-app.h"
#include "ns3/uinteger.h"

namespace ns3
{

ServerNodeAppHelper::ServerNodeAppHelper(uint16_t port, string nodeName, string configFilename, int nodesQuantity)
    : ApplicationHelper(ServerNodeApp::GetTypeId())
{
    SetAttribute("Port", UintegerValue(port));
    SetAttribute("NodeName", StringValue(nodeName));
    SetAttribute("ConfigFilename", StringValue(configFilename));
    SetAttribute("TotalNodesQuantity", IntegerValue(nodesQuantity));
}

ClientNodeAppHelper::ClientNodeAppHelper(vector<Ipv4Address>& addresses, uint16_t port, string nodeName, string configFilename, int nodesQuantity)
    : ApplicationHelper(ClientNodeApp::GetTypeId())
{
    SetAttribute("RemoteAddresses", StringValue(utils::convertAddressesToString(addresses)));
    SetAttribute("RemotePort", UintegerValue(port));
    SetAttribute("NodeName", StringValue(nodeName));
    SetAttribute("ConfigFilename", StringValue(configFilename));
    SetAttribute("TotalNodesQuantity", IntegerValue(nodesQuantity));
}

// ClientNodeAppHelper::ClientNodeAppHelper(vector<Ipv4Address>& addresses, string nodeName, string configFilename)
//     : ApplicationHelper(ClientNodeApp::GetTypeId())
// {
//     SetAttribute("Addresses", StringValue(utils::convertAddressesToString(addresses)));
//     SetAttribute("NodeName", StringValue(nodeName));
//     SetAttribute("ConfigFilename", StringValue(configFilename));
// }

} // namespace ns3
