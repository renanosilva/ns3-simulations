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

#ifndef SENSOR_AND_CLIENT_HELPER_H
#define SENSOR_AND_CLIENT_HELPER_H

#include <ns3/application-helper.h>
#include "ns3/utils.h"
#include <stdint.h>
#include <string>

using namespace std;

namespace ns3
{

/**
 * \ingroup server
 * \brief Cria uma aplicação referente a um nó que opera a bateria (como, por exemplo, um sensor).
 */
class ServerNodeAppHelper : public ApplicationHelper
{
  public:
    /**
     * Create ServerNodeAppHelper which will make life easier for people trying
     * to set up simulations with nodes that operate with batteries.
     *
     * \param port The port the server will wait on for incoming packets
     */
    ServerNodeAppHelper(uint16_t port, string nodeName, string configFilename);
};

/**
 * \ingroup server
 * \brief Create an application which sends packets and waits for response
 */
class ClientNodeAppHelper : public ApplicationHelper
{
  public:
    /**
     * Create ClientNodeAppHelper which will make life easier for people trying
     * to set up simulations with nodes that communicate with another node. 
     *
     * \param ip The IP address of the remote sensor
     * \param port The port number of the remote sensor
     */
    ClientNodeAppHelper(vector<Ipv4Address>& addresses, uint16_t port, string nodeName, string configFilename);

    /**
     * Create ClientHelper which will make life easier for people trying
     * to set up simulations with servers that operate with batteries. 
     * Use this variant with addresses that do
     * include a port value (e.g., InetSocketAddress and Inet6SocketAddress).
     *
     * \param addr The address of the remote sensor
     */
    // ClientNodeAppHelper(vector<Ipv4Address>& addresses, string nodeName, string configFilename);

};

} // namespace ns3

#endif /* UDP_ECHO_HELPER_H */
