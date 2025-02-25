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
#include "ns3/ipv4.h"
#include "ns3/ipv4-address.h"

#include <cstdio>
#include <cstdlib>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ClientApp");
NS_OBJECT_ENSURE_REGISTERED(ClientNodeApp);

TypeId
ClientNodeApp::GetTypeId()
{
    NS_LOG_FUNCTION("ClientNodeApp::GetTypeId()");

    static TypeId tid =
        TypeId("ns3::ClientNodeApp")
            .SetParent<CheckpointApp>()
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
            .AddAttribute("Address",
                            "The node's address",
                            AddressValue(Ipv4Address::GetAny()),
                            MakeAddressAccessor(&ClientNodeApp::m_address),
                            MakeAddressChecker())
            .AddAttribute("Port",
                            "The node's port",
                            UintegerValue(0),
                            MakeUintegerAccessor(&ClientNodeApp::m_port),
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

    NS_LOG_FUNCTION("Fim do método");

    return tid;
}

ClientNodeApp::ClientNodeApp()
{
    NS_LOG_FUNCTION(this);
    
    m_sent = 0;
    m_totalTx = 0;
    m_socket = nullptr;
    m_sendEvent = EventId();

    m_address = Ipv4Address::GetAny();
    m_port = 0;

    defineCheckpointStrategy();

    NS_LOG_FUNCTION("Fim do método");
}

ClientNodeApp::~ClientNodeApp()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_FUNCTION("Fim do método");
}

void
ClientNodeApp::SetRemote(Address ip, uint16_t port)
{
    NS_LOG_FUNCTION(this << ip << port);
    m_peerAddress = ip;
    m_peerPort = port;
    NS_LOG_FUNCTION("Fim do método");
}

void
ClientNodeApp::SetRemote(Address addr)
{
    NS_LOG_FUNCTION(this << addr);
    m_peerAddress = addr;
    NS_LOG_FUNCTION("Fim do método");
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
            // Se for a primeira execução, o ns-3 irá escolher o IP
            if (m_address == Ipv4Address::GetAny())
            {
                //NS_LOG_INFO("PRIMEIRO START");
                
                //m_socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), m_port));
                m_socket->Bind();
                
                // Após o Bind(), recuperar o IP atribuído pelo ns-3
                Address a;
                m_socket->GetSockName(a);  // Obtém o endereço local do socket
                InetSocketAddress localAddr = InetSocketAddress::ConvertFrom(a);
                m_port = localAddr.GetPort();   // Obtém a porta local
                //m_address = localAddr.GetIpv4();  // Obtém o IP local

                m_address = GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
            }
            else
            {
                //NS_LOG_INFO("OUTRO START");

                // Nas execuções subsequentes, usar o IP previamente atribuído
                m_socket->Bind(InetSocketAddress(Ipv4Address::ConvertFrom(m_address), m_port));
            }

            m_socket->SetIpTos(m_tos);
            m_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
        else if (InetSocketAddress::IsMatchingType(m_peerAddress))
        {
            if (m_address == Ipv4Address::GetAny())
            {
                m_socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), m_port));
                m_address = GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
            }
            else
            {
                m_socket->Bind(InetSocketAddress(Ipv4Address::ConvertFrom(m_address), m_port));
            }

            m_socket->SetIpTos(m_tos);
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
    NS_LOG_FUNCTION("Fim do método");
}

void
ClientNodeApp::StopApplication()
{
    NS_LOG_FUNCTION(this);
    
    Simulator::Cancel(m_sendEvent);
    m_socket->ShutdownRecv();
    m_socket->Close();

    NS_LOG_FUNCTION("Fim do método");
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

    NS_LOG_FUNCTION("Fim do método");
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

            if (command == NORMAL_PAYLOAD){
                last_seq = data;
            }

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

    NS_LOG_FUNCTION("Fim do método");
}

void ClientNodeApp::notifyNodesAboutRollbackConcluded(){
    NS_LOG_FUNCTION(this);
    
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
                    << " porta " << m_peerPort
                    << " o seguinte comando: " << data);

    NS_LOG_FUNCTION("Fim do método");
}

void ClientNodeApp::logMessageReceived(Ptr<Packet> packet, Address from, uint32_t currentSequenceNumber, string command, int data){
    NS_LOG_FUNCTION(this << packet << from << currentSequenceNumber << command << data);

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

    NS_LOG_FUNCTION("Fim do método");
}

void ClientNodeApp::resetNodeData() {
    NS_LOG_FUNCTION(this);

    printNodeData();
    
    StopApplication();
    
    m_socket = nullptr;

    //m_sendEvent.Cancel();
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

    checkpointStrategy = nullptr;
    //delete checkpointStrategy;

    NS_LOG_FUNCTION("Fim do método");
}

void ClientNodeApp::defineCheckpointStrategy() {
    NS_LOG_FUNCTION(this);
    
    //checkpointStrategy = new SyncPredefinedTimesCheckpoint(Seconds(5.0), getNodeName(), this);
    checkpointStrategy = Create<SyncPredefinedTimesCheckpoint>(Seconds(5.0), getNodeName(), this);
    checkpointStrategy->startCheckpointing();

    NS_LOG_FUNCTION("Fim do método");
}

void ClientNodeApp::beforeRollback(){
    NS_LOG_FUNCTION(this);
    NS_LOG_FUNCTION("Fim do método");
}

void ClientNodeApp::afterRollback(){
    NS_LOG_FUNCTION(this);

    //Reiniciando aplicação...
    StartApplication();

    printNodeData();

    notifyNodesAboutRollbackConcluded();

    NS_LOG_FUNCTION("Fim do método");
}

string ClientNodeApp::getNodeName(){
    NS_LOG_FUNCTION(this);
    return "client-node-0";
}

uint64_t
ClientNodeApp::GetTotalTx() const
{
    NS_LOG_FUNCTION(this);
    return m_totalTx;
}

json ClientNodeApp::to_json() const {
    NS_LOG_FUNCTION(this);

    json j = Application::to_json();
    j["m_count"] = m_count;
    j = timeToJson(j, "m_interval", m_interval);
    j["m_size"] = m_size;
    j["m_sent"] = m_sent;
    j["last_seq"] = last_seq;
    j["m_totalTx"] = m_totalTx;
    j["m_address"] = m_address;
    j["m_port"] = m_port;
    j["m_peerAddress"] = m_peerAddress;
    j["m_peerPort"] = m_peerPort;
    j["m_tos"] = m_tos;
    
    //j["m_sendEvent"] = m_sendEvent;
    //j = checkpointStrategyToJson(j, checkpointStrategy);

    NS_LOG_FUNCTION("Fim do método");

    return j;
}

void ClientNodeApp::from_json(const json& j) {
    NS_LOG_FUNCTION(this);
    
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

    if (j.contains("m_address") && !j["m_address"].is_null()) {
        j.at("m_address").get_to(m_address);
    } else {
        NS_LOG_INFO("\nm_address está ausente ou é nulo! \n");
    }

    if (j.contains("m_peerAddress") && !j["m_peerAddress"].is_null()) {
        j.at("m_peerAddress").get_to(m_peerAddress);
    } else {
        NS_LOG_INFO("\nm_peerAddress está ausente ou é nulo! \n");
    }

    if (j.contains("m_port") && !j["m_port"].is_null()) {
        j.at("m_port").get_to(m_port);
    } else {
        NS_LOG_INFO("\nm_port está ausente ou é nulo! \n");
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

    NS_LOG_FUNCTION("Fim do método");

    //j.at("idleEnergyConsumption").get_to(idleEnergyConsumption);  // Desserializa o membro da classe derivada
}

void ClientNodeApp::printNodeData(){
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO("\nDados de " << getNodeName() << ":" );
    NS_LOG_INFO(
        "m_txTrace.IsEmpty() = " << m_txTrace.IsEmpty()
        << ", m_rxTrace.IsEmpty() = " << m_rxTrace.IsEmpty()
        << ", m_txTraceWithAddresses.IsEmpty() = " << m_txTraceWithAddresses.IsEmpty()
        << ", m_rxTraceWithAddresses.IsEmpty() = " << m_rxTraceWithAddresses.IsEmpty()
        << ", m_count = " << m_count
        << ", m_interval = " << m_interval.As(Time::S)
        << ", m_size = " << m_size
        << ", m_sent = " << m_sent
        << ", last_seq = " << last_seq
        << ", m_totalTx = " << m_totalTx
        << ", m_address = " << Ipv4Address::ConvertFrom(m_address)
        << ", m_port = " << m_port
        << ", m_peerAddress = " << Ipv4Address::ConvertFrom(m_peerAddress)
        << ", m_peerPort = " << m_peerPort
        << ", m_tos = " << to_string(static_cast<int>(m_tos))
        << ", m_socket->GetAllowBroadcast() = " << m_socket->GetAllowBroadcast()
        << ", m_sendEvent = " << m_sendEvent.GetTs()
        << "\n ");

    NS_LOG_FUNCTION("Fim do método");
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
