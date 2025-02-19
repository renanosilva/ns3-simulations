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

#include "client-node-app.h"

#include "seq-ts-header.h"

#include "ns3/sync-predefined-times-checkpoint.h"

#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/ipv4-address.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/uinteger.h"

#include <cstdio>
#include <cstdlib>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ClientApp");

NS_OBJECT_ENSURE_REGISTERED(ClientNodeApp);

TypeId
ClientNodeApp::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ClientNodeApp")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<ClientNodeApp>()
            .AddAttribute(
                "MaxPackets",
                "The maximum number of packets the application will send (zero means infinite)",
                UintegerValue(100),
                MakeUintegerAccessor(&ClientNodeApp::m_count),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute("Interval",
                          "The time to wait between packets",
                          TimeValue(Seconds(1.0)),
                          MakeTimeAccessor(&ClientNodeApp::m_interval),
                          MakeTimeChecker())
            .AddAttribute("RemoteAddress",
                          "The destination Address of the outbound packets",
                          AddressValue(),
                          MakeAddressAccessor(&ClientNodeApp::m_peerAddress),
                          MakeAddressChecker())
            .AddAttribute("RemotePort",
                          "The destination port of the outbound packets",
                          UintegerValue(0),
                          MakeUintegerAccessor(&ClientNodeApp::m_peerPort),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("Tos",
                          "The Type of Service used to send IPv4 packets. "
                          "All 8 bits of the TOS byte are set (including ECN bits).",
                          UintegerValue(0),
                          MakeUintegerAccessor(&ClientNodeApp::m_tos),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("PacketSize",
                          "Size of packets generated. The minimum packet size is 12 bytes which is "
                          "the size of the header carrying the sequence number and the time stamp.",
                          UintegerValue(1024),
                          MakeUintegerAccessor(&ClientNodeApp::m_size),
                          MakeUintegerChecker<uint32_t>(12, 65507))
            .AddTraceSource("Tx",
                            "A new packet is created and is sent",
                            MakeTraceSourceAccessor(&ClientNodeApp::m_txTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("TxWithAddresses",
                            "A new packet is created and is sent",
                            MakeTraceSourceAccessor(&ClientNodeApp::m_txTraceWithAddresses),
                            "ns3::Packet::TwoAddressTracedCallback");
    return tid;
}

ClientNodeApp::ClientNodeApp()
{
    NS_LOG_FUNCTION(this);
    m_sent = 0;
    m_totalTx = 0;
    m_socket = nullptr;
    m_sendEvent = EventId();

    checkpointStrategy = new SyncPredefinedTimesCheckpoint(Seconds(5.0), "client-node-0", this);
    checkpointStrategy->startCheckpointing();
}

ClientNodeApp::~ClientNodeApp()
{
    NS_LOG_FUNCTION(this);
}

void
ClientNodeApp::SetRemote(Address ip, uint16_t port)
{
    NS_LOG_FUNCTION(this << ip << port);
    m_peerAddress = ip;
    m_peerPort = port;
}

void
ClientNodeApp::SetRemote(Address addr)
{
    NS_LOG_FUNCTION(this << addr);
    m_peerAddress = addr;
}

void
ClientNodeApp::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (!m_socket)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);
        NS_ABORT_MSG_IF(m_peerAddress.IsInvalid(), "'RemoteAddress' attribute not properly set");
        if (Ipv4Address::IsMatchingType(m_peerAddress))
        {
            if (m_socket->Bind() == -1)
            {
                NS_FATAL_ERROR("Failed to bind socket");
            }
            m_socket->SetIpTos(m_tos); // Affects only IPv4 sockets.
            m_socket->Connect(
                InetSocketAddress(Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
        else if (InetSocketAddress::IsMatchingType(m_peerAddress))
        {
            if (m_socket->Bind() == -1)
            {
                NS_FATAL_ERROR("Failed to bind socket");
            }
            m_socket->SetIpTos(m_tos); // Affects only IPv4 sockets.
            m_socket->Connect(m_peerAddress);
        }
        else
        {
            NS_ASSERT_MSG(false, "Incompatible address type: " << m_peerAddress);
        }
    }

    m_socket->SetRecvCallback(MakeCallback(&ClientNodeApp::HandleRead, this));
    m_socket->SetAllowBroadcast(true);
    m_sendEvent = Simulator::Schedule(Seconds(0.0), &ClientNodeApp::Send, this);

    NS_LOG_INFO("Cliente iniciado.");
}

void
ClientNodeApp::StopApplication()
{
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(m_sendEvent);
}

void
ClientNodeApp::Send()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_sendEvent.IsExpired());

    Address from;
    Address to;
    m_socket->GetSockName(from);
    m_socket->GetPeerName(to);

    /* Cabeçalho do pacote a ser enviado */

    SeqTsHeader seqTs;
    seqTs.SetSeq(m_sent);
    NS_ABORT_IF(m_size < seqTs.GetSerializedSize());

    /* Payload (corpo) do pacote */

    // Serializar dados: comando (string) + número inteiro (int)
    ostringstream oss;
    oss << NORMAL_PAYLOAD << " " << 0;

    string data = oss.str();

    /* Criação do pacote com o conteúdo a ser enviado */
    //Ptr<Packet> p = Create<Packet>(m_size - seqTs.GetSerializedSize());
    Ptr<Packet> p = Create<Packet>((const uint8_t*) data.c_str(), data.length());

    // Trace before adding header, for consistency with PacketSink
    m_txTrace(p);
    m_txTraceWithAddresses(p, from, to);

    p->AddHeader(seqTs);

    if ((m_socket->Send(p)) >= 0)
    {
        ++m_sent;
        m_totalTx += p->GetSize();

    }

    if (Ipv4Address::IsMatchingType(m_peerAddress))
    {
        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << " cliente enviou " << m_size
                               << " bytes para " << Ipv4Address::ConvertFrom(m_peerAddress)
                               << " porta " << m_peerPort
                               << " (Número de Sequência: " << seqTs.GetSeq()
                               << ", UId: " << p->GetUid() << ")");
        
        //addCheckpointData(true, m_size, from, seqTs.GetSeq(), p->GetUid());

    }
    else if (InetSocketAddress::IsMatchingType(m_peerAddress))
    {
        NS_LOG_INFO(
            "Aos " << Simulator::Now().As(Time::S) << " cliente enviou " << m_size << " bytes para "
                       << InetSocketAddress::ConvertFrom(m_peerAddress).GetIpv4() << " porta "
                       << InetSocketAddress::ConvertFrom(m_peerAddress).GetPort());
        
        //addCheckpointData(true, m_size, m_peerAddress, seqTs.GetSeq(), p->GetUid());
    }

    if (m_sent < m_count || m_count == 0)
    {
        m_sendEvent = Simulator::Schedule(m_interval, &ClientNodeApp::Send, this);
    }

}

void
ClientNodeApp::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    Ptr<Packet> packet;
    Address from;
    Address localAddress;
    while ((packet = socket->RecvFrom(from)))
    {
        if (InetSocketAddress::IsMatchingType(from))
        {
            SeqTsHeader seqTs;
            packet->RemoveHeader(seqTs);
            uint32_t currentSequenceNumber = seqTs.GetSeq();

            //Obtendo payload do pacote
            uint8_t buffer[1024];
            packet->CopyData(buffer, packet->GetSize());
            
            //uint8_t buffer[sizeof(last_seq)];
            //packet->CopyData(buffer, sizeof(last_seq));

            // Convertendo o buffer de volta para uint64_t
            //memcpy(&last_seq, buffer, sizeof(last_seq));

            //Obtendo os dados do payload
            string receivedData(reinterpret_cast<char*>(buffer), packet->GetSize());

            istringstream iss(receivedData);
            string command;
            int data;
            iss >> command >> data;

            logMessageReceived(packet, from, currentSequenceNumber, command, data);

            if (command == REQUEST_TO_START_ROLLBACK_COMMAND){
                resetNodeData();
                defineCheckpointStrategy();
                checkpointStrategy->startRollback(data);
            }
            
            //addCheckpointData(false, packet->GetSize(), from, -1, -1);
        }
        
        socket->GetSockName(localAddress);
        m_rxTrace(packet);
        m_rxTraceWithAddresses(packet, from, localAddress);
    }
}

void ClientNodeApp::notifyNodesAboutRollbackConcluded(){
    /* Cabeçalho do pacote a ser enviado */

    SeqTsHeader seqTs;
    seqTs.SetSeq(0);
    
    /* Payload (corpo) do pacote */

    // Serializar dados: comando (string) + número inteiro (int)
    ostringstream oss;
    oss << ROLLBACK_FINISHED << " " << checkpointStrategy->getLastCheckpointId();

    string data = oss.str();

    /* Criação do pacote com o conteúdo a ser enviado */
    Ptr<Packet> packet = Create<Packet>((const uint8_t*) data.c_str(), data.length());
    packet->AddHeader(seqTs);

    // Enviando o pacote para o destino
    m_socket->Send(packet);
    
    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << getNodeName() 
                    << " enviou para "
                    << Ipv4Address::ConvertFrom(m_peerAddress)
                    << " o seguinte comando: " << data);
}

void ClientNodeApp::logMessageReceived(Ptr<Packet> packet, Address from, uint32_t currentSequenceNumber, string command, int data){

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << " cliente recebeu "
                                   << packet->GetSize() << " bytes de "
                                   << InetSocketAddress::ConvertFrom(from).GetIpv4() << " porta "
                                   << InetSocketAddress::ConvertFrom(from).GetPort()
                                   << " (Número de Sequência: " << currentSequenceNumber
                                   << ", UId: " << packet->GetUid() 
                                   << (command == NORMAL_PAYLOAD ? "" : ", Comando: " + command)
                                   << (command == NORMAL_PAYLOAD ? ", last_seq: " + to_string(data) 
                                                                    : ", Dado: " + to_string(data))
                                   << ")");
}

void ClientNodeApp::resetNodeData() {
    m_socket->ShutdownRecv();
    m_socket->Close();
    m_socket = nullptr;

    m_sendEvent.Cancel();
    m_sendEvent = EventId();

    m_count = 0;
    m_interval = Time(); 
    m_size = 0; 
    m_sent = 0;
    last_seq = 0;
    m_totalTx = 0;
    m_peerAddress = Address();
    m_peerPort = 0;
    m_tos = 0;

    delete checkpointStrategy;
}

void ClientNodeApp::defineCheckpointStrategy() {
    checkpointStrategy = new SyncPredefinedTimesCheckpoint(Seconds(5.0), getNodeName(), this);
    checkpointStrategy->startCheckpointing();
}

void ClientNodeApp::beforeRollback(){
    
}

void ClientNodeApp::afterRollback(){
    
    //Reiniciando aplicação...
    StartApplication();

    notifyNodesAboutRollbackConcluded();

}

string ClientNodeApp::getNodeName(){
    return "client-node-0";
}

uint64_t
ClientNodeApp::GetTotalTx() const
{
    return m_totalTx;
}

json ClientNodeApp::to_json() const {
    
    json j = Application::to_json();
    j["m_count"] = m_count;
    j = timeToJson(j, "m_interval", m_interval);
    j["m_size"] = m_size;
    j["m_sent"] = m_sent;
    j["last_seq"] = last_seq;
    j["m_totalTx"] = m_totalTx;
    j["m_peerAddress"] = m_peerAddress;
    j["m_peerPort"] = m_peerPort;
    j["m_tos"] = m_tos;
    
    //j["m_sendEvent"] = m_sendEvent;
    //j = checkpointStrategyToJson(j, checkpointStrategy);

    return j;
}

void ClientNodeApp::from_json(const json& j) {
    CheckpointApp::from_json(j);

    if (j.contains("m_count") && !j["m_count"].is_null()) {
        j.at("m_count").get_to(m_count); 
    } else {
        NS_LOG_INFO("\nm_count está ausente ou é nulo! \n");
    }

    if (j.contains("m_interval") && !j["m_interval"].is_null()) {
        m_interval = jsonToTime(j, "m_interval"); 
    } else {
        NS_LOG_INFO("\nm_interval está ausente ou é nulo! \n");
    }

    if (j.contains("m_size") && !j["m_size"].is_null()) {
        j.at("m_size").get_to(m_size);
    } else {
        NS_LOG_INFO("\nm_size está ausente ou é nulo! \n");
    }

    if (j.contains("m_sent") && !j["m_sent"].is_null()) {
        j.at("m_sent").get_to(m_sent);
    } else {
        NS_LOG_INFO("\nm_sent está ausente ou é nulo! \n");
    }

    if (j.contains("last_seq") && !j["last_seq"].is_null()) {
        j.at("last_seq").get_to(last_seq);
    } else {
        NS_LOG_INFO("\nlast_seq está ausente ou é nulo! \n");
    }

    if (j.contains("m_totalTx") && !j["m_totalTx"].is_null()) {
        j.at("m_totalTx").get_to(m_totalTx);
    } else {
        NS_LOG_INFO("\nm_totalTx está ausente ou é nulo! \n");
    }

    if (j.contains("m_peerAddress") && !j["m_peerAddress"].is_null()) {
        j.at("m_peerAddress").get_to(m_peerAddress);
    } else {
        NS_LOG_INFO("\nm_peerAddress está ausente ou é nulo! \n");
    }

    if (j.contains("m_peerPort") && !j["m_peerPort"].is_null()) {
        j.at("m_peerPort").get_to(m_peerPort);
    } else {
        NS_LOG_INFO("\nm_peerPort está ausente ou é nulo! \n");
    }

    if (j.contains("m_tos") && !j["m_tos"].is_null()) {
        j.at("m_tos").get_to(m_tos);
    } else {
        NS_LOG_INFO("\nm_tos está ausente ou é nulo! \n");
    }

    //j.at("idleEnergyConsumption").get_to(idleEnergyConsumption);  // Desserializa o membro da classe derivada
}

/*void ClientNodeApp::addCheckpointData(bool sent, uint32_t bytes, Address from, uint32_t seqNumber, uint64_t uid) {
    checkpointStrategy->addCheckpointData(std::to_string(Simulator::Now().GetSeconds()) + "|" +
                                                        (sent ? "sent" : "received") + "|" +
                                                        to_string(bytes) + "|" +
                                                        InetSocketAddress::ConvertFrom(from).GetIpv4().toString() + "|" +
                                                        to_string(InetSocketAddress::ConvertFrom(from).GetPort()) + "|" +
                                                        to_string(seqNumber) + "|" +
                                                        to_string(uid) + "\n");
}*/

} // Namespace ns3
