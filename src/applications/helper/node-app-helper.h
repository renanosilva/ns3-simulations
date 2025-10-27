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
 * \ingroup ECS
 * \brief Classe responsável por criar uma aplicação referente a uma ECS (Estação Central de Suporte), que centraliza toda a comunicação
 * do sistema.
 */
class ECSNodeAppHelper : public ApplicationHelper
{
  public:
    /**
     * Create ServerNodeAppHelper which will make life easier for people trying
     * to set up simulations with nodes that operate with batteries.
     *
     */
    ECSNodeAppHelper(uint16_t ecsPort, string nodeName, string configFilename, int nodesQuantity);
};

/**
 * \ingroup ECS
 * \brief Classe responsável por criar uma aplicação referente a um nó com papel de Responsor em um cenário de Sistema Distribuído
 * com ECS (Estação Central de Suporte), que centraliza toda a comunicação do sistema. O Responsor apenas responde requisições
 * que chegam a ele, ou seja, ele não tem iniciativa de enviar requisições a outros nós, a não ser para responder mensagens.
 */
class ResponsorNodeAppHelper : public ApplicationHelper
{
  public:
    
    ResponsorNodeAppHelper(uint16_t serverPort, string nodeName, Ipv4Address& ecsAddress, uint16_t ecsPort, string configFilename, int nodesQuantity);

};

/**
 * \ingroup ECS
 * \brief Classe responsável por criar uma aplicação referente a um nó com papel de Requestor em um cenário de Sistema Distribuído
 * com ECS (Estação Central de Suporte), que centraliza toda a comunicação do sistema. O Requestor envia requisições a outros nós
 * do sistema e aguarda as respostas.
 */
class RequestorNodeAppHelper : public ApplicationHelper
{
  public:
    
    RequestorNodeAppHelper(vector<Ipv4Address>& serverAddresses, uint16_t serverPort, Ipv4Address& ecsAddress, uint16_t ecsPort, 
                          string nodeName, string configFilename, int nodesQuantity);
    
};

/**
 * \ingroup server
 * \brief Cria uma aplicação referente a um nó que atua como um servidor, recebendo e respondendo requisições.
 */
class ServerNodeAppHelper : public ApplicationHelper
{
  public:
    
    ServerNodeAppHelper(uint16_t port, string nodeName, string configFilename, int nodesQuantity);
    
};

/**
 * \ingroup client
 * \brief Cria uma aplicação que envia pacotes e aguarda por resposta.
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
    ClientNodeAppHelper(vector<Ipv4Address>& addresses, uint16_t port, string nodeName, string configFilename, int nodesQuantity);

};

} // namespace ns3

#endif /* UDP_ECHO_HELPER_H */
