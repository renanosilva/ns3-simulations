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

#include "sensor-app.h"
#include "fixed-energy-generator.h"
//#include "circular-energy-generator.h"

#include "ns3/address-utils.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/uinteger.h"
#include "ns3/node-depleted-exception.h"
#include "ns3/node-asleep-exception.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BatteryServerApp");

NS_OBJECT_ENSURE_REGISTERED(SensorApp);

static const double CONNECT_ENERGY = 100.; //Energia gasta para conectar um socket
static const double SEND_ENERGY = 100.; //Energia gasta para enviar um pacote
static const double RECEIVE_ENERGY = 100.; //Energia gasta para receber um pacote

TypeId
SensorApp::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SensorApp")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<SensorApp>()
            .AddAttribute("Port",
                          "Port on which we listen for incoming packets.",
                          UintegerValue(9),
                          MakeUintegerAccessor(&SensorApp::m_port),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("Tos",
                          "The Type of Service used to send IPv4 packets. "
                          "All 8 bits of the TOS byte are set (including ECN bits).",
                          UintegerValue(0),
                          MakeUintegerAccessor(&SensorApp::m_tos),
                          MakeUintegerChecker<uint8_t>())
            .AddTraceSource("Rx",
                            "A packet has been received",
                            MakeTraceSourceAccessor(&SensorApp::m_rxTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("RxWithAddresses",
                            "A packet has been received",
                            MakeTraceSourceAccessor(&SensorApp::m_rxTraceWithAddresses),
                            "ns3::Packet::TwoAddressTracedCallback");
    return tid;
}

SensorApp::SensorApp()
{
    NS_LOG_FUNCTION(this);

    energyUpdateInterval = Seconds(1.0);
    idleEnergyConsumption = 10;
    sleepEnergyConsumption = 1;
    currentMode = NORMAL;

    energyGenerator = new FixedEnergyGenerator(40);
    //energyGenerator = new CircularEnergyGenerator();

    NS_LOG_INFO("Iniciando nó a bateria... Energia inicial: " << battery.getRemainingEnergy());

    //Agendando geração de energia
    Simulator::Schedule(energyUpdateInterval,
                                &SensorApp::generateEnergy,
                                this);

    //Agendando desconto contínuo de energia referente ao funcionamento básico do dispositivo
    Simulator::Schedule(energyUpdateInterval,
                                &SensorApp::decreaseCurrentModeEnergy,
                                this);
}

SensorApp::~SensorApp()
{
    NS_LOG_FUNCTION(this);
    m_socket = nullptr;
    m_socket6 = nullptr;

    delete energyGenerator;
}

void
SensorApp::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (!m_socket)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
        if (m_socket->Bind(local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
        if (addressUtils::IsMulticast(m_local))
        {
            Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket>(m_socket);
            if (udpSocket)
            {
                // equivalent to setsockopt (MCAST_JOIN_GROUP)
                udpSocket->MulticastJoinGroup(0, m_local);
            }
            else
            {
                NS_FATAL_ERROR("Error: Failed to join multicast group");
            }
        }
    }

    if (!m_socket6)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket6 = Socket::CreateSocket(GetNode(), tid);
        Inet6SocketAddress local6 = Inet6SocketAddress(Ipv6Address::GetAny(), m_port);
        if (m_socket6->Bind(local6) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
        if (addressUtils::IsMulticast(local6))
        {
            Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket>(m_socket6);
            if (udpSocket)
            {
                // equivalent to setsockopt (MCAST_JOIN_GROUP)
                udpSocket->MulticastJoinGroup(0, local6);
            }
            else
            {
                NS_FATAL_ERROR("Error: Failed to join multicast group");
            }
        }
    }

    m_socket->SetIpTos(m_tos); // Affects only IPv4 sockets.
    m_socket->SetRecvCallback(MakeCallback(&SensorApp::HandleRead, this));
    m_socket6->SetRecvCallback(MakeCallback(&SensorApp::HandleRead, this));

    NS_LOG_INFO("Nó servidor a bateria conectado.");
    decreaseConnectEnergy();
}

void
SensorApp::StopApplication()
{
    NS_LOG_FUNCTION(this);

    if (m_socket)
    {
        m_socket->Close();
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
    if (m_socket6)
    {
        m_socket6->Close();
        m_socket6->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
}

void
SensorApp::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    Ptr<Packet> packet;
    Address from;
    Address localAddress;
    while ((packet = socket->RecvFrom(from)) && !isSleeping() && !isDepleted())
    {
        try {

            socket->GetSockName(localAddress);
            m_rxTrace(packet);
            m_rxTraceWithAddresses(packet, from, localAddress);
            if (InetSocketAddress::IsMatchingType(from))
            {
                NS_LOG_INFO("\nAos " << Simulator::Now().As(Time::S) << " servidor recebeu "
                                    << packet->GetSize() << " bytes de "
                                    << InetSocketAddress::ConvertFrom(from).GetIpv4() << " porta "
                                    << InetSocketAddress::ConvertFrom(from).GetPort());
            }
            else if (Inet6SocketAddress::IsMatchingType(from))
            {
                NS_LOG_INFO("\nAos " << Simulator::Now().As(Time::S) << " servidor recebeu "
                                    << packet->GetSize() << " bytes de "
                                    << Inet6SocketAddress::ConvertFrom(from).GetIpv6() << " porta "
                                    << Inet6SocketAddress::ConvertFrom(from).GetPort());
            }

            /*uint8_t *buffer = new uint8_t[packet->GetSize()];
            uint32_t size = packet->CopyData(buffer, packet->GetSize());
            string s = string(buffer, buffer+packet->GetSize());
            NS_LOG_INFO("Received:" << s);*/

            /*std::ostringstream oss;
            packet->Print(oss);
            NS_LOG_INFO(oss.str());*/

            decreaseReadEnergy();

            packet->RemoveAllPacketTags();
            packet->RemoveAllByteTags();

            NS_LOG_LOGIC("Echoing packet");
            socket->SendTo(packet, 0, from);

            if (InetSocketAddress::IsMatchingType(from))
            {
                NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << " servidor enviou "
                                    << packet->GetSize() << " bytes para "
                                    << InetSocketAddress::ConvertFrom(from).GetIpv4() << " porta "
                                    << InetSocketAddress::ConvertFrom(from).GetPort());
            }
            else if (Inet6SocketAddress::IsMatchingType(from))
            {
                NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << " servidor enviou "
                                    << packet->GetSize() << " bytes para "
                                    << Inet6SocketAddress::ConvertFrom(from).GetIpv6() << " porta "
                                    << Inet6SocketAddress::ConvertFrom(from).GetPort());
            }

            decreaseSendEnergy();

        } catch (NodeAsleepException& e) {
            //NS_LOG_INFO("Tarefa incompleta por estar em modo SLEEP.");
            continue;
        } catch (NodeDepletedException& e) {
            //NS_LOG_INFO("Tarefa incompleta por estar em modo SLEEP.");
            continue;
        } 
        
    }
}

Mode SensorApp::getCurrentMode(){
    return currentMode;
}

bool SensorApp::isSleeping(){
    return currentMode == SLEEP;
}

bool SensorApp::isDepleted(){
    return currentMode == DEPLETED;
}

Time SensorApp::getEnergyUpdateInterval(){
    return energyUpdateInterval;
}

void SensorApp::generateEnergy(){
    battery.rechargeEnergy(energyGenerator->getValue());

    checkModeChange();

    Simulator::Schedule(energyUpdateInterval,
                                &SensorApp::generateEnergy,
                                this);
}

void SensorApp::checkModeChange(){
    if (battery.getBatteryPercentage() == 0 && currentMode != Mode::DEPLETED){
        currentMode = Mode::DEPLETED;

        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", nó entrou em modo DEPLETED.");

        throw NodeDepletedException("Bateria entrou em modo DEPLETED.");

    } else if (battery.getBatteryPercentage() <= 10 && currentMode == Mode::NORMAL){
        currentMode = Mode::SLEEP;

        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", nó entrou em modo SLEEP.");

        throw NodeAsleepException("Bateria entrou em modo SLEEP.");

    } else if (battery.getBatteryPercentage() > 10 && currentMode != Mode::NORMAL){
        currentMode = Mode::NORMAL;

        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", nó entrou em modo NORMAL.");
    }
}

void SensorApp::decreaseEnergy(double amount) {
    NS_ASSERT_MSG(
            (currentMode == Mode::SLEEP && amount == sleepEnergyConsumption) ||
            (currentMode == Mode::NORMAL), 
                    "Operação inválida! Bateria está em modo sleep e não pode realizar operações.");
    
    battery.decrementEnergy(amount);

    checkModeChange();
}

void SensorApp::decreaseReadEnergy(){
    decreaseEnergy(RECEIVE_ENERGY);
}

void SensorApp::decreaseSendEnergy(){
    decreaseEnergy(SEND_ENERGY);
}

void SensorApp::decreaseConnectEnergy(){
    decreaseEnergy(CONNECT_ENERGY);
}

void SensorApp::decreaseIdleEnergy(){
    try {
        decreaseEnergy(idleEnergyConsumption);
    } catch (NodeAsleepException& e) {
        //Nada a fazer
    } catch (NodeDepletedException& e) {
        //Nada a fazer
    } 
}

void SensorApp::decreaseSleepEnergy(){
    try {
        decreaseEnergy(sleepEnergyConsumption);
    } catch (NodeDepletedException& e) {
        //Nada a fazer
    } 
}

void SensorApp::decreaseCurrentModeEnergy(){
    if (currentMode == Mode::NORMAL){
        decreaseIdleEnergy();
    } else if (currentMode == Mode::SLEEP){
        decreaseSleepEnergy();
    }

    Simulator::Schedule(energyUpdateInterval,
                                &SensorApp::decreaseCurrentModeEnergy,
                                this);
}

/*int Battery::generateRandomNumber(double min, double max){
    // Initialize a random number generator
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> distrib(min, max);

    // Generate random number in the range [min, max]
    return distrib(gen);
}*/

} // Namespace ns3
