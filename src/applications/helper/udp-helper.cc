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
 * Author: Geoge Riley <riley@ece.gatech.edu>
 * Adapted from OnOffHelper by:
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "udp-helper.h"

namespace ns3
{

UdpHelper::UdpHelper(Ptr<Node> node, uint16_t port)
{
    m_node = node;
    m_port = port;
    
    // Criar socket UDP no nó
    m_socket = Socket::CreateSocket(m_node, UdpSocketFactory::GetTypeId());
    
    // Configurar o endereço de escuta do socket
    InetSocketAddress localAddress = InetSocketAddress(Ipv4Address::GetAny(), m_port);
    m_socket->Bind(localAddress);
    
    // Configurar o callback de recebimento de pacotes
    //m_socket->SetRecvCallback(MakeCallback(&UdpHelper::ReceivePacket, this));
}

// Enviar um pacote UDP para o endereço IP e porta de destino
void UdpHelper::SendPacket(Ipv4Address destAddr, uint16_t destPort, const std::string &data)
{
    // Criar o pacote com o conteúdo (dados) a ser enviado
    Ptr<Packet> packet = Create<Packet>((const uint8_t*)data.c_str(), data.length());

    // Criar o endereço de destino
    InetSocketAddress destAddress = InetSocketAddress(destAddr, destPort);
    
    // Enviar o pacote para o destino
    m_socket->SendTo(packet, 0, destAddress);

    NS_LOG_UNCOND("Pacote enviado para " << destAddr << ":" << destPort << " com dados: " << data);
}

// Função de callback para receber pacotes
void UdpHelper::ReceivePacket(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;
    
    // Receber o pacote
    while ((packet = socket->RecvFrom(from)))
    {
        NS_LOG_UNCOND("Pacote recebido de " << InetSocketAddress::ConvertFrom(from).GetIpv4() << " tamanho: " << packet->GetSize());
        // Aqui você pode processar o pacote conforme necessário
    }
}

} // namespace ns3
