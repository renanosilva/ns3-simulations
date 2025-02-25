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

#include "battery-node-app.h"

#include "packet-loss-counter.h"
#include "seq-ts-header.h"

#include "fixed-energy-generator.h"
#include "circular-energy-generator.h"

#include "ns3/sync-predefined-times-checkpoint.h"

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
#include "ns3/uinteger.h"
#include "ns3/node-depleted-exception.h"
#include "ns3/node-asleep-exception.h"
#include "ns3/enum.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BatteryServerApp");

NS_OBJECT_ENSURE_REGISTERED(BatteryNodeApp);

static const double CONNECT_ENERGY = 100.; //Energia gasta para conectar um socket
static const double SEND_ENERGY = 100.; //Energia gasta para enviar um pacote
static const double RECEIVE_ENERGY = 100.; //Energia gasta para receber um pacote
static const double CREATE_CHECKPOINT_ENERGY = 100.; //Energia gasta para criar um checkpoint
static const double IDLE_ENERGY = 10.; //Energia gasta para se manter ligado (funcionamento básico)
static const double SLEEP_ENERGY = 1.; //Energia gasta para se manter em modo sleep (funcionamento básico)
static const Time ENERGY_UPDATE_INTERVAL = Seconds(1.0); //Intervalo de atualização de energia da bateria

TypeId
BatteryNodeApp::GetTypeId()
{
    NS_LOG_FUNCTION("BatteryNodeApp::GetTypeId()");

    static TypeId tid =
        TypeId("ns3::BatteryNodeApp")
            .SetParent<CheckpointApp>()
            .SetGroupName("Applications")
            .AddConstructor<BatteryNodeApp>()
            .AddAttribute("Port",
                          "Port on which we listen for incoming packets.",
                          UintegerValue(9),
                          MakeUintegerAccessor(&BatteryNodeApp::m_port),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("Tos",
                          "The Type of Service used to send IPv4 packets. "
                          "All 8 bits of the TOS byte are set (including ECN bits).",
                          UintegerValue(0),
                          MakeUintegerAccessor(&BatteryNodeApp::m_tos),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("PacketWindowSize",
                          "The size of the window used to compute the packet loss. This value "
                          "should be a multiple of 8.",
                          UintegerValue(32),
                          MakeUintegerAccessor(&BatteryNodeApp::GetPacketWindowSize,
                                               &BatteryNodeApp::SetPacketWindowSize),
                          MakeUintegerChecker<uint16_t>(8, 256))
            .AddTraceSource("Rx",
                            "A packet has been received",
                            MakeTraceSourceAccessor(&BatteryNodeApp::m_rxTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("RxWithAddresses",
                            "A packet has been received",
                            MakeTraceSourceAccessor(&BatteryNodeApp::m_rxTraceWithAddresses),
                            "ns3::Packet::TwoAddressTracedCallback");
    
    NS_LOG_FUNCTION("Fim do método");

    return tid;
}

BatteryNodeApp::BatteryNodeApp()
    : m_received(0),
    m_seq(-1),
    m_lossCounter(0)
{

    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Iniciando " << getNodeName() << "... Energia inicial: " << battery.getRemainingEnergy());

    energyUpdateInterval = ENERGY_UPDATE_INTERVAL;
    idleEnergyConsumption = IDLE_ENERGY;
    sleepEnergyConsumption = SLEEP_ENERGY;
    currentMode = NORMAL;
    rollbackInProgress = false;
    m_local = Address();
    addresses.clear();

    defineCheckpointStrategy();
    defineEnergyGenerator();

    //Agendando geração de energia
    Simulator::Schedule(energyUpdateInterval,
                                &BatteryNodeApp::generateEnergy,
                                this);

    //Agendando desconto contínuo de energia referente ao funcionamento básico do dispositivo
    Simulator::Schedule(energyUpdateInterval,
                                &BatteryNodeApp::decreaseCurrentModeEnergy,
                                this);
    
    NS_LOG_FUNCTION("Fim do método");
}

BatteryNodeApp::~BatteryNodeApp()
{
    NS_LOG_FUNCTION(this);
    m_socket = nullptr;

    addresses.clear();

    //delete energyGenerator;
    //delete checkpointStrategy;

    NS_LOG_FUNCTION("Fim do método");
}

void BatteryNodeApp::defineCheckpointStrategy() {
    NS_LOG_FUNCTION(this);
    
    //checkpointStrategy = new SyncPredefinedTimesCheckpoint(Seconds(5.0), getNodeName(), this);
    checkpointStrategy = Create<SyncPredefinedTimesCheckpoint>(Seconds(5.0), getNodeName(), this);
    checkpointStrategy->startCheckpointing();

    NS_LOG_FUNCTION("Fim do método");
}

void BatteryNodeApp::defineEnergyGenerator() {
    NS_LOG_FUNCTION(this);
    
    //energyGenerator = new FixedEnergyGenerator(100);
    //energyGenerator = new CircularEnergyGenerator();
    energyGenerator = Create<CircularEnergyGenerator>();

    NS_LOG_FUNCTION("Fim do método");
}

uint16_t
BatteryNodeApp::GetPacketWindowSize() const
{
    NS_LOG_FUNCTION(this);
    return m_lossCounter.GetBitMapSize();
}

void
BatteryNodeApp::SetPacketWindowSize(uint16_t size)
{
    NS_LOG_FUNCTION(this << size);
    m_lossCounter.SetBitMapSize(size);
    NS_LOG_FUNCTION("Fim do método");
}

uint32_t
BatteryNodeApp::GetLost() const
{
    NS_LOG_FUNCTION(this);
    return m_lossCounter.GetLost();
}

uint64_t
BatteryNodeApp::GetReceived() const
{
    NS_LOG_FUNCTION(this);
    return m_received;
}

void
BatteryNodeApp::StartApplication()
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

    /*if (!m_socket6)
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
    }*/

    m_socket->SetIpTos(m_tos); // Affects only IPv4 sockets.
    m_socket->SetRecvCallback(MakeCallback(&BatteryNodeApp::HandleRead, this));
    //m_socket6->SetRecvCallback(MakeCallback(&BatteryNodeApp::HandleRead, this));

    NS_LOG_INFO(getNodeName() << " conectado.");
    decreaseConnectEnergy();

    NS_LOG_FUNCTION("Fim do método");
}

void
BatteryNodeApp::StopApplication()
{
    NS_LOG_FUNCTION(this);

    if (m_socket)
    {
        m_socket->Close();
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
    /*if (m_socket6)
    {
        m_socket6->Close();
        m_socket6->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }*/

    NS_LOG_FUNCTION("Fim do método");
}

void
BatteryNodeApp::HandleRead(Ptr<Socket> socket)
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

            if (packet->GetSize() > 0)
            {
                uint32_t receivedSize = packet->GetSize();
                SeqTsHeader seqTs;
                packet->RemoveHeader(seqTs);
                uint32_t currentSequenceNumber = seqTs.GetSeq();

                //Obtendo payload do pacote
                uint8_t buffer[1024];
                packet->CopyData(buffer, packet->GetSize());

                std::string receivedData(reinterpret_cast<char*>(buffer), packet->GetSize());

                istringstream iss(receivedData);
                string command;
                int data;
                iss >> command >> data;

                if (rollbackInProgress && command != ROLLBACK_FINISHED){
                    //Ignora pacotes até a conclusão do rollback dos outros nós
                    continue;
                }

                if (rollbackInProgress && command == ROLLBACK_FINISHED){
                    NS_LOG_INFO("\nAos " << Simulator::Now().As(Time::S) << ", " << getNodeName() 
                                    << " saiu do modo de bloqueio de comunicação.");
                    
                    rollbackInProgress = false;
                    decreaseReadEnergy();
                    continue;
                }

                NS_LOG_INFO("\nAos " << Simulator::Now().As(Time::S) << " servidor recebeu "
                                    << receivedSize << " bytes de "
                                    << InetSocketAddress::ConvertFrom(from).GetIpv4() << " porta "
                                    << InetSocketAddress::ConvertFrom(from).GetPort()
                                    << " (Número de Sequência: " << currentSequenceNumber
                                    << ", UId: " << packet->GetUid() 
                                    /*<< ", TXtime: " << seqTs.GetTs().As(Time::S) 
                                    << ", RXtime: " << Simulator::Now().As(Time::S)
                                    << ", Delay: " << (Simulator::Now() - seqTs.GetTs()).As(Time::S)*/
                                    << (command == NORMAL_PAYLOAD ? "" : ", Comando: " + command)
                                    << (command == NORMAL_PAYLOAD && data > 0 ? ", last_seq: " + to_string(data) : "")
                                    << (command != NORMAL_PAYLOAD ? ", Dado: " + to_string(data) : "") 
                                    << ")");

                //addCheckpointData(false, receivedSize, from, currentSequenceNumber, packet->GetUid());
                
                /*checkpointStrategy->addCheckpointData(to_string(Simulator::Now().GetSeconds()) + "|" +
                                                        to_string(receivedSize) + "|" +
                                                        InetSocketAddress::ConvertFrom(from).GetIpv4().toString() + "|" +
                                                        to_string(InetSocketAddress::ConvertFrom(from).GetPort()) + "|" +
                                                        to_string(currentSequenceNumber) + "|" +
                                                        to_string(packet->GetUid()) + "\n");*/

                /*else if (Inet6SocketAddress::IsMatchingType(from))
                {
                    NS_LOG_INFO("\nAos " << Simulator::Now().As(Time::S) << " servidor recebeu "
                                        << receivedSize << " bytes de "
                                        << Inet6SocketAddress::ConvertFrom(from).GetIpv6() << " porta "
                                        << Inet6SocketAddress::ConvertFrom(from).GetPort()
                                        << " (Número de Sequência: " << currentSequenceNumber
                                        << ", UId: " << packet->GetUid() 
                                        << ", TXtime: " << seqTs.GetTs().As(Time::S) 
                                        << ", RXtime: " << Simulator::Now().As(Time::S)
                                        << ", Delay: " << (Simulator::Now() - seqTs.GetTs()).As(Time::S)
                                        << ")");
                }*/

                m_lossCounter.NotifyReceived(currentSequenceNumber);
                m_received++;
                m_seq++;

                decreaseReadEnergy();

                packet->RemoveAllPacketTags();
                packet->RemoveAllByteTags();

                NS_LOG_LOGIC("Responding packet");

                /* Payload (corpo) do pacote */

                // Serializar dados: comando (string) + número inteiro (int)
                ostringstream oss;
                oss << NORMAL_PAYLOAD << " " << m_seq;

                string dataToSend = oss.str();

                /* Criação do pacote com o conteúdo a ser enviado */
                //Ptr<Packet> p = Create<Packet>(m_size - seqTs.GetSerializedSize());
                Ptr<Packet> newPacket = Create<Packet>((const uint8_t*) dataToSend.c_str(), dataToSend.length());
                newPacket->AddHeader(seqTs);

                /*uint64_t dataToSend = m_seq;

                // Convertendo o dado para uint8_t* (ponteiro para bytes)
                uint8_t* buffer = reinterpret_cast<uint8_t*>(&dataToSend);

                Ptr<Packet> newPacket = Create<Packet>(buffer, sizeof(dataToSend) + seqTs.GetSerializedSize());
                newPacket->AddHeader(seqTs);*/

                socket->SendTo(newPacket, 0, from);

                if (InetSocketAddress::IsMatchingType(from))
                {
                    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << " servidor enviou "
                                        << receivedSize << " bytes para "
                                        << InetSocketAddress::ConvertFrom(from).GetIpv4() << " porta "
                                        << InetSocketAddress::ConvertFrom(from).GetPort()
                                        << " (Número de Sequência: " << seqTs.GetSeq()
                                        << ", UId: " << newPacket->GetUid() 
                                        << ", last_seq: " << m_seq << ")");
                }

                /*else if (Inet6SocketAddress::IsMatchingType(from))
                {
                    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << " servidor enviou "
                                        << receivedSize << " bytes para "
                                        << Inet6SocketAddress::ConvertFrom(from).GetIpv6() << " porta "
                                        << Inet6SocketAddress::ConvertFrom(from).GetPort());
                }*/

                //addCheckpointData(true, receivedSize, from, -1, -1);

                addAddress(from);

                decreaseSendEnergy();

            }
        } catch (NodeAsleepException& e) {
            //NS_LOG_INFO("Tarefa incompleta por estar em modo SLEEP.");
            break;
        } catch (NodeDepletedException& e) {
            //NS_LOG_INFO("Tarefa incompleta por estar em modo DEPLETED.");
            break;
        }
        
    }

    NS_LOG_FUNCTION("Fim do método");
}

Mode BatteryNodeApp::getCurrentMode(){
    NS_LOG_FUNCTION(this);
    return currentMode;
}

bool BatteryNodeApp::isSleeping(){
    NS_LOG_FUNCTION(this);
    return currentMode == SLEEP;
}

bool BatteryNodeApp::isDepleted(){
    NS_LOG_FUNCTION(this);
    return currentMode == DEPLETED;
}

Time BatteryNodeApp::getEnergyUpdateInterval(){
    NS_LOG_FUNCTION(this);
    return energyUpdateInterval;
}

void BatteryNodeApp::generateEnergy(){
    NS_LOG_FUNCTION(this);
    
    battery.rechargeEnergy(energyGenerator->getValue());

    checkModeChange();

    Simulator::Schedule(energyUpdateInterval,
                                &BatteryNodeApp::generateEnergy,
                                this);

    NS_LOG_FUNCTION("Fim do método");
}

void BatteryNodeApp::checkModeChange(){
    NS_LOG_FUNCTION(this);
    
    if (battery.getBatteryPercentage() == 0 && currentMode != Mode::DEPLETED){
        currentMode = Mode::DEPLETED;

        resetNodeData();

        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << getNodeName() << " entrou em modo DEPLETED.");
        throw NodeDepletedException("Bateria entrou em modo DEPLETED.");

    } else if (battery.getBatteryPercentage() <= 10 && currentMode == Mode::NORMAL){
        currentMode = Mode::SLEEP;

        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << getNodeName() << " entrou em modo SLEEP.");

        resetNodeData();

        throw NodeAsleepException("Bateria entrou em modo SLEEP.");

    } else if (battery.getBatteryPercentage() > 20 && currentMode != Mode::NORMAL){
        /*NS_LOG_INFO("Socket: " << (m_socket ? true : false) << ", Porta: " << m_port <<
                    ", TOS: " << m_tos << ", m_local: " << m_local.GetLength() << 
                    ", m_received: " << m_received << ", m_seq :" << m_seq <<
                    ", m_lossCounter: " << m_lossCounter.GetLost() <<
                    ", checkpointStrategy: " << checkpointStrategy);*/

        currentMode = Mode::NORMAL;
        defineCheckpointStrategy();

        NS_LOG_INFO("\nAos " << Simulator::Now().As(Time::S) << ", " << getNodeName() << " entrou em modo NORMAL.");

        checkpointStrategy->startRollbackToLastCheckpoint();

        /*NS_LOG_INFO("Socket: " << (m_socket ? true : false) << ", Porta: " << m_port <<
            ", TOS: " << m_tos << ", m_local: " << m_local.GetLength() << 
            ", m_received: " << m_received << ", m_seq :" << m_seq <<
            ", m_lossCounter: " << m_lossCounter.GetLost() <<
            ", checkpointStrategy: " << checkpointStrategy);*/
    }

    NS_LOG_FUNCTION("Fim do método");
}

void BatteryNodeApp::beforeCheckpoint(){
    NS_LOG_FUNCTION(this);
}

void BatteryNodeApp::afterCheckpoint(){
    NS_LOG_FUNCTION(this);
    
    //Removendo endereços com os quais este nó se comunicou no ciclo anterior
    addresses.clear();

    NS_LOG_FUNCTION("Fim do método");
}

void BatteryNodeApp::beforeRollback(){
    NS_LOG_FUNCTION(this);
    
    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << getNodeName() 
                                    << " entrou no modo de bloqueio de comunicação.");
    rollbackInProgress = true;

    NS_LOG_FUNCTION("Fim do método");
}

void BatteryNodeApp::afterRollback(){
    NS_LOG_FUNCTION(this);

    //Reiniciando aplicação...
    StartApplication();

    printNodeData();

    //Enviando mensagem de rollback para os outros nós envolvidos
    notifyNodesAboutRollback();

    NS_LOG_FUNCTION("Fim do método");
}

void BatteryNodeApp::notifyNodesAboutRollback(){
    NS_LOG_FUNCTION(this);

    /* Cabeçalho do pacote a ser enviado */

    SeqTsHeader seqTs;
    seqTs.SetSeq(0);
    
    /* Payload (corpo) do pacote */

    // Serializar dados: comando (string) + número inteiro (int)
    ostringstream oss;
    oss << REQUEST_TO_START_ROLLBACK_COMMAND << " " << checkpointStrategy->getLastCheckpointId();

    string data = oss.str();

    /* Criação do pacote com o conteúdo a ser enviado */
    Ptr<Packet> packet = Create<Packet>((const uint8_t*) data.c_str(), data.length());
    packet->AddHeader(seqTs);

    // Enviando para os nós com os quais houve comunicação

    if (addresses.size() > 0){

        for (Address a : addresses){
            // Enviando o pacote para o destino
            m_socket->SendTo(packet, 0, a);
            NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << getNodeName() 
                            << " enviou para "
                            << InetSocketAddress::ConvertFrom(a).GetIpv4() 
                            << " porta "
                            << InetSocketAddress::ConvertFrom(a).GetPort()
                            << " o seguinte comando: " << data);
        }
    
    }

    NS_LOG_FUNCTION("Fim do método");
}

void BatteryNodeApp::decreaseEnergy(double amount) {
    NS_LOG_FUNCTION(this);
    
    NS_ASSERT_MSG(
            (currentMode == Mode::SLEEP && amount == sleepEnergyConsumption) ||
            (currentMode == Mode::NORMAL), 
                    "Operação inválida! Bateria está em modo sleep e não pode realizar operações.");
    
    battery.decrementEnergy(amount);

    checkModeChange();

    NS_LOG_FUNCTION("Fim do método");
}

void BatteryNodeApp::decreaseCheckpointEnergy(){
    NS_LOG_FUNCTION(this);
    decreaseEnergy(CREATE_CHECKPOINT_ENERGY);
    NS_LOG_FUNCTION("Fim do método");
}

void BatteryNodeApp::decreaseReadEnergy(){
    NS_LOG_FUNCTION(this);
    decreaseEnergy(RECEIVE_ENERGY);
    NS_LOG_FUNCTION("Fim do método");
}

void BatteryNodeApp::decreaseSendEnergy(){
    NS_LOG_FUNCTION(this);
    decreaseEnergy(SEND_ENERGY);
    NS_LOG_FUNCTION("Fim do método");
}

void BatteryNodeApp::decreaseConnectEnergy(){
    NS_LOG_FUNCTION(this);
    decreaseEnergy(CONNECT_ENERGY);
    NS_LOG_FUNCTION("Fim do método");
}

void BatteryNodeApp::decreaseIdleEnergy(){
    NS_LOG_FUNCTION(this);

    try {
        decreaseEnergy(idleEnergyConsumption);
    } catch (NodeAsleepException& e) {
        //Nada a fazer
    } catch (NodeDepletedException& e) {
        //Nada a fazer
    } 

    NS_LOG_FUNCTION("Fim do método");
}

void BatteryNodeApp::decreaseSleepEnergy(){
    NS_LOG_FUNCTION(this);
    
    try {
        decreaseEnergy(sleepEnergyConsumption);
    } catch (NodeDepletedException& e) {
        //Nada a fazer
    }

    NS_LOG_FUNCTION("Fim do método");
}

void BatteryNodeApp::decreaseCurrentModeEnergy(){
    NS_LOG_FUNCTION(this);
    
    if (currentMode == Mode::NORMAL){
        decreaseIdleEnergy();
    } else if (currentMode == Mode::SLEEP){
        decreaseSleepEnergy();
    }

    Simulator::Schedule(energyUpdateInterval,
                                &BatteryNodeApp::decreaseCurrentModeEnergy,
                                this);
    
    NS_LOG_FUNCTION("Fim do método");
}

void BatteryNodeApp::resetNodeData() {
    NS_LOG_FUNCTION(this);

    printNodeData();

    StopApplication();
    m_socket = nullptr;

    //m_socket->ShutdownRecv();
    //m_socket->Close();

    addresses.clear();

    m_port = 0;
    m_tos = 0;
    m_local = Address();
    m_received = 0;
    m_seq = -1;
    m_lossCounter = PacketLossCounter(0);
    rollbackInProgress = false;

    checkpointStrategy = nullptr;
    //delete checkpointStrategy;

    NS_LOG_FUNCTION("Fim do método");
}

void BatteryNodeApp::addAddress(Address a){
    NS_LOG_FUNCTION(this);
    
    auto it = find(addresses.begin(), addresses.end(), a);

    if (it == addresses.end()) {
        //Endereço não foi encontrado
        //Adiciona endereço ao vetor de endereços
        addresses.push_back(a);
    }

    NS_LOG_FUNCTION("Fim do método");
}

void BatteryNodeApp::printNodeData(){
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO("\nDados de " << getNodeName() << ":" );
    NS_LOG_INFO(
        "m_rxTrace.IsEmpty() = " << m_rxTrace.IsEmpty()
        << ", m_rxTraceWithAddresses.IsEmpty() = " << m_rxTraceWithAddresses.IsEmpty()
        << ", battery.getBatteryPercentage() = " << battery.getBatteryPercentage()
        << ", currentMode = " << currentMode
        << ", idleEnergyConsumption = " << idleEnergyConsumption
        << ", sleepEnergyConsumption = " << sleepEnergyConsumption
        << ", energyUpdateInterval = " << energyUpdateInterval.As(Time::S)
        << ", energyGenerator->GetTypeId() = " << energyGenerator->GetTypeId()
        << ", rollbackInProgress = " << rollbackInProgress
        << ", m_socket->GetAllowBroadcast() = " << m_socket->GetAllowBroadcast()
        << ", m_port = " << m_port
        << ", m_tos = " << to_string(static_cast<int>(m_tos))
        << ", m_local.IsInvalid() = " << m_local.IsInvalid()
        << ", m_received = " << m_received
        << ", m_seq = " << m_seq
        << ", m_lossCounter = " << m_lossCounter.GetLost()
        << ", addresses.size() = " << addresses.size()
        << "\n ");

    NS_LOG_FUNCTION("Fim do método");
}

json BatteryNodeApp::to_json() const {
    NS_LOG_FUNCTION(this);

    json j = Application::to_json();
    j["m_port"] = m_port;
    j["m_tos"] = m_tos;
    j["m_received"] = m_received;
    j["m_seq"] = m_seq;
    j["m_local"] = m_local;
    j["m_lossCounter"] = m_lossCounter;
    j["addresses"] = addresses;

    //j = checkpointStrategyToJson(j, checkpointStrategy);

    //j["sleepEnergyConsumption"] = sleepEnergyConsumption;
    //j["idleEnergyConsumption"] = idleEnergyConsumption;
    //j["currentMode"] = currentMode;
    //j["battery"] = battery;
    //j = timeToJson(j, "energyUpdateInterval", energyUpdateInterval);
    //j = energyGeneratorToJson(j, energyGenerator);

    NS_LOG_FUNCTION("Fim do método");

    return j;
}

void BatteryNodeApp::from_json(const json& j) {
    NS_LOG_FUNCTION(this);
    
    CheckpointApp::from_json(j);

    if (j.contains("m_port") && !j["m_port"].is_null()) {
        j.at("m_port").get_to(m_port);
    } else {
        NS_LOG_INFO("\nm_port está ausente ou é nulo! \n");
    }

    if (j.contains("m_tos") && !j["m_tos"].is_null()) {
        j.at("m_tos").get_to(m_tos); 
    } else {
        NS_LOG_INFO("\nm_tos está ausente ou é nulo! \n");
    }

    if (j.contains("m_local") && !j["m_local"].is_null()) {
        j.at("m_local").get_to(m_local); 
    } else {
        NS_LOG_INFO("\nm_local está ausente ou é nulo! \n");
    }

    if (j.contains("m_received") && !j["m_received"].is_null()) {
        j.at("m_received").get_to(m_received); 
    } else {
        NS_LOG_INFO("\nm_received está ausente ou é nulo! \n");
    }

    if (j.contains("m_seq") && !j["m_seq"].is_null()) {
        j.at("m_seq").get_to(m_seq); 
    } else {
        NS_LOG_INFO("\nm_seq está ausente ou é nulo! \n");
    }

    if (j.contains("m_lossCounter") && !j["m_lossCounter"].is_null()) {
        j.at("m_lossCounter").get_to(m_lossCounter); 
    } else {
        NS_LOG_INFO("\nm_lossCounter está ausente ou é nulo! \n");
    }

    if (j.contains("addresses") && !j["addresses"].is_null()) {
        j.at("addresses").get_to(addresses); 
    } else {
        NS_LOG_INFO("\naddresses está ausente ou é nulo! \n");
    }

    NS_LOG_FUNCTION("Fim do método");
}

string BatteryNodeApp::getNodeName(){
    NS_LOG_FUNCTION(this);

    return "battery-node-0";
}

/*json BatteryNodeApp::socketToJson(json j) const {
    UdpSocket* us = dynamic_cast<UdpSocket*>(m_socket.operator->());

    if (us) {
        j["m_socket"] = *us;
    } else {
        j["m_socket"] = *m_socket;
    }

    return j;
}*/

/*void BatteryNodeApp::addLog(bool sent, uint32_t bytes, Address from, uint32_t seqNumber, uint64_t uid) {
    checkpointStrategy->addLogData(to_string(Simulator::Now().GetSeconds()) + "|" +
                                                        (sent ? "sent" : "received") + "|" +
                                                        to_string(bytes) + "|" +
                                                        InetSocketAddress::ConvertFrom(from).GetIpv4().toString() + "|" +
                                                        to_string(InetSocketAddress::ConvertFrom(from).GetPort()) + "|" +
                                                        to_string(seqNumber) + "|" +
                                                        to_string(uid) + "\n");
}*/

} // Namespace ns3
