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
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/ipv4-address.h"
#include "ns3/packet.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-address.h"
#include "ns3/core-module.h"
#include "ns3/seq-ts-header.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UDPHelper");
NS_OBJECT_ENSURE_REGISTERED(UDPHelper);

TypeId
UDPHelper::GetTypeId()
{
    NS_LOG_FUNCTION("UDPHelper::GetTypeId()");

    static TypeId tid =
        TypeId("ns3::UDPHelper")
            .SetParent<Object>()
            .SetGroupName("Helpers")
            .AddConstructor<UDPHelper>()
            .AddAttribute("Address",
                            "The node's address",
                            AddressValue(Ipv4Address::GetAny()),
                            MakeAddressAccessor(&UDPHelper::m_address),
                            MakeAddressChecker())
            .AddAttribute("Port",
                            "The node's port",
                            UintegerValue(0),
                            MakeUintegerAccessor(&UDPHelper::m_port),
                            MakeUintegerChecker<uint16_t>())
            .AddAttribute("TotalTX",
                            "Total bytes sent",
                            UintegerValue(0),
                            MakeUintegerAccessor(&UDPHelper::m_totalTx),
                            MakeUintegerChecker<uint64_t>())
            .AddAttribute("MessagesSent",
                            "Counter for sent messages",
                            UintegerValue(0),
                            MakeUintegerAccessor(&UDPHelper::m_sent),
                            MakeUintegerChecker<uint32_t>())
            .AddAttribute("MessagesReceived",
                            "Counter for received messages",
                            UintegerValue(0),
                            MakeUintegerAccessor(&UDPHelper::m_received),
                            MakeUintegerChecker<uint32_t>())
            .AddTraceSource("Tx",
                            "A new packet is created and is sent",
                            MakeTraceSourceAccessor(&UDPHelper::m_txTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("TxWithAddresses",
                            "A new packet is created and is sent",
                            MakeTraceSourceAccessor(&UDPHelper::m_txTraceWithAddresses),
                            "ns3::Packet::TwoAddressTracedCallback");

    return tid;
}

UDPHelper::UDPHelper()
{
    NS_LOG_FUNCTION(this);
    
    m_socket = nullptr;

    m_address = Ipv4Address::GetAny();
    m_port = 0;
    m_local = Address();
    m_totalTx = 0;
    m_sent = 0;
    m_received = 0;
}

UDPHelper::~UDPHelper()
{
    NS_LOG_FUNCTION(this);

    m_socket = nullptr;
}

void UDPHelper::configureClient(Ptr<Node> node, string nodeName){
    NS_LOG_FUNCTION(this << node << nodeName);
    
    m_nodeName = nodeName;

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    m_socket = Socket::CreateSocket(node, tid);
    
    // Se for a primeira execução, o ns-3 irá escolher o IP
    if (m_address == Ipv4Address::GetAny()) {
        
        m_socket->Bind();
        
        // Após o Bind(), recuperar o IP atribuído pelo ns-3
        Address a;
        m_socket->GetSockName(a);  // Obtém o endereço local do socket
        InetSocketAddress localAddr = InetSocketAddress::ConvertFrom(a);
        m_port = localAddr.GetPort();   // Obtém a porta local
        m_address = node->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
    }
    else {
        // Nas execuções subsequentes, usar o IP previamente atribuído
        m_socket->Bind(InetSocketAddress(Ipv4Address::ConvertFrom(m_address), m_port));
    }

    m_socket->SetRecvCallback(MakeCallback(&UDPHelper::HandleRead, this));
}

void UDPHelper::configureServer(Ptr<Node> node, string nodeName, uint16_t port){
    NS_LOG_FUNCTION(this << node << nodeName << port);
    
    m_nodeName = nodeName;
    m_port = port;

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    m_socket = Socket::CreateSocket(node, tid);
    
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
    
    if (m_socket->Bind(local) == -1) {
        NS_FATAL_ERROR("Failed to bind socket");
    }

    // if (addressUtils::IsMulticast(m_local)) {
    //     Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket>(m_socket);
    //     if (udpSocket) {
    //         // equivalent to setsockopt (MCAST_JOIN_GROUP)
    //         udpSocket->MulticastJoinGroup(0, m_local);
    //     } else {
    //         NS_FATAL_ERROR("Error: Failed to join multicast group");
    //     }
    // }

    m_socket->SetRecvCallback(MakeCallback(&UDPHelper::HandleRead, this));
}

void UDPHelper::terminateConnection(){
    NS_LOG_FUNCTION(this);
    
    m_socket->ShutdownRecv();
    m_socket->Close();
}

Ptr<MessageData> UDPHelper::send(string command, int d, Address to){
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(!to.IsInvalid(), "Endereço de envio de pacote inválido!");

    Address from;
    m_socket->GetSockName(from);
    
    /* Cabeçalho do pacote a ser enviado */
    
    SeqTsHeader seqTs;
    seqTs.SetSeq(m_sent);

    /* Payload (corpo) do pacote */

    // Serializando dados: comando (string) + número inteiro (int)
    ostringstream oss;
    oss << command << " " << d;

    string data = oss.str();

    /* Criação do pacote com o conteúdo a ser enviado */
    Ptr<Packet> p = Create<Packet>((const uint8_t*) data.c_str(), data.length());

    // Trace before adding header, for consistency with PacketSink
    m_txTrace(p);
    m_txTraceWithAddresses(p, from, to);

    p->AddHeader(seqTs);

    Ptr<MessageData> md = Create<MessageData>();
    md->SetUid(p->GetUid());
    md->SetFrom(from);
    md->SetTo(to);
    md->SetSequenceNumber(seqTs.GetSeq());
    md->SetCommand(command);
    md->SetData(d);
    md->SetSize(p->GetSize());

    bool intercepted = false;

    if (!protocolSendCallback.IsNull()){
        //Dando a oportunidade de o protocolo de checkpointing interceptar a mensagem
        intercepted = protocolSendCallback(md);
    }
    
    //Só envia o pacote caso o protocolo de checkpointing permita
    if (!intercepted){
        //Enviando pacote
        if (m_socket->SendTo(p, 0, to)){
            
            ++m_sent;
            m_totalTx += p->GetSize();
        }
    }

    return md;
}

Ptr<MessageData> UDPHelper::send(string command, int d, Ipv4Address ip, uint16_t port){
    InetSocketAddress destination = InetSocketAddress(ip, port);
    return send(command, d, destination);
}

void UDPHelper::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    Ptr<Packet> packet;
    Address from;
    Address localAddress;
    Ptr<MessageData> md;

    while ((packet = socket->RecvFrom(from)))
    {
        if (InetSocketAddress::IsMatchingType(from))
        {
            uint32_t receivedSize = packet->GetSize();
            
            SeqTsHeader seqTs;
            packet->RemoveHeader(seqTs);
            uint32_t currentSequenceNumber = seqTs.GetSeq();

            //Obtendo payload do pacote
            uint8_t buffer[1024];
            packet->CopyData(buffer, packet->GetSize());
            
            //Obtendo os dados do payload
            string receivedData(reinterpret_cast<char*>(buffer), packet->GetSize());

            istringstream iss(receivedData);
            string command;
            int data;
            iss >> command >> data;

            md = Create<MessageData>();
            md->SetUid(packet->GetUid());
            md->SetCommand(command);
            md->SetData(data);
            md->SetFrom(from);
            md->SetSequenceNumber(currentSequenceNumber);
            md->SetSize(receivedSize);
        }

        socket->GetSockName(localAddress);
        m_rxTrace(packet);
        m_rxTraceWithAddresses(packet, from, localAddress);

        md->SetTo(localAddress);
    }

    m_received++;
    
    if (!protocolReceiveCallback.IsNull()){
        //Dando a oportunidade de o protocolo de checkpointing interceptar a mensagem antes da aplicação
        bool result = protocolReceiveCallback(md);
        
        //Se o protocolo de checkpointing não tiver processado a mensagem
        if (!result){
            //Então dá oportunidade à aplicação de processar a mensagem
            receiveCallback(md);
        }
    } else {
        receiveCallback(md);
    }
}

void UDPHelper::printData(){
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO(
        "m_txTrace.IsEmpty() = " << m_txTrace.IsEmpty()
        << ", m_rxTrace.IsEmpty() = " << m_rxTrace.IsEmpty()
        << ", m_txTraceWithAddresses.IsEmpty() = " << m_txTraceWithAddresses.IsEmpty()
        << ", m_rxTraceWithAddresses.IsEmpty() = " << m_rxTraceWithAddresses.IsEmpty()
        << ", m_sent = " << m_sent
        << ", m_received = " << m_received
        << ", m_totalTx = " << m_totalTx
        << ", m_address = " << Ipv4Address::ConvertFrom(m_address)
        << ", m_port = " << m_port
        << ", m_local.IsInvalid() = " << m_local.IsInvalid()
        << ", m_socket->GetAllowBroadcast() = " << m_socket->GetAllowBroadcast()
        << "\n ");
}

void to_json(json& j, const UDPHelper& obj) {
    NS_LOG_FUNCTION("UDPHelper::to_json");
    
    j = nlohmann::json{
        {"m_address", obj.m_address},
        {"m_port", obj.m_port},
        {"m_local", obj.m_local},
        {"m_totalTx", obj.m_totalTx}, 
        {"m_sent", obj.m_sent},
        {"m_received", obj.m_received},
        {"m_nodeName", obj.m_nodeName}
    };
}

void from_json(const json& j, UDPHelper& obj) {
    NS_LOG_FUNCTION("UDPHelper::from_json");
    
    j.at("m_address").get_to(obj.m_address);
    j.at("m_port").get_to(obj.m_port);
    j.at("m_local").get_to(obj.m_local);
    j.at("m_totalTx").get_to(obj.m_totalTx);
    j.at("m_sent").get_to(obj.m_sent);
    j.at("m_received").get_to(obj.m_received);
    j.at("m_nodeName").get_to(obj.m_nodeName);
}

// Getters
Address UDPHelper::getAddress() const { return m_address; }
uint16_t UDPHelper::getPort() const { return m_port; }
uint64_t UDPHelper::getTotalBytesSent() const { return m_totalTx; }
uint32_t UDPHelper::getSentMessagesCounter() const { return m_sent; }
uint32_t UDPHelper::getReceivedMessagesCounter() const { return m_received; }
string UDPHelper::getNodeName() const { return m_nodeName; }
bool UDPHelper::isDisconnected() const { return !m_socket; }

// Setters
void UDPHelper::setAddress(const ns3::Address& address) { m_address = address; }
void UDPHelper::setPort(uint16_t port) { m_port = port; }
void UDPHelper::setNodeName(const std::string& name) { m_nodeName = name; }

void UDPHelper::setReceiveCallback(Callback<void, Ptr<MessageData>> callback){
    receiveCallback = callback;
}

void UDPHelper::setProtocolReceiveCallback(Callback<bool, Ptr<MessageData>> callback){
    protocolReceiveCallback = callback;
}

void UDPHelper::setProtocolSendCallback(Callback<bool, Ptr<MessageData>> callback){
    protocolSendCallback = callback;
}


} // namespace ns3
