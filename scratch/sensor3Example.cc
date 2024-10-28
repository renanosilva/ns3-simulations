/*
 * Copyright (c) 2014 Wireless Communications and Networking Group (WCNG),
 * University of Rochester, Rochester, NY, USA.
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
 */

/**
 *
 * This example extends the energy model example by connecting a basic energy
 * harvester to the nodes.
 *
 * The example considers a simple communication link between a source and a
 * destination node, where the source node sends a packet to the destination
 * every 1 second. Each node is powered by a BasicEnergySource, which is recharged
 * by a BasicEnergyHarvester, and the WiFi radio consumes energy for the transmission/
 * reception of the packets.
 *
 * For the receiver node, the example prints the energy consumption of the WiFi radio,
 * the power harvested by the energy harvester and the residual energy in the
 * energy source.
 *
 * The nodes initial energy is set to 1 J, the transmission and reception entail a
 * current consumption of 0.0174 A and 0.0197 A, respectively (default values in
 * WifiRadioEnergyModel). The energy harvester provides an amount of power that varies
 * according to a random variable uniformly distributed in [0 0.1] W, and is updated
 * every 1 s. The energy source voltage is 3 V (default value in BasicEnergySource) and
 * the residual energy level is updated every 1 second (default value).
 *
 * The simulation start at time 0 and it is hard stopped at time 10 seconds. Given the
 * packet size and the distance between the nodes, each transmission lasts 0.0023s.
 * As a result, the destination node receives 10 messages.
 *
 */

#include "ns3/core-module.h"
#include "ns3/energy-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/yans-wifi-helper.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace ns3;
using namespace ns3::energy;

NS_LOG_COMPONENT_DEFINE("Sensor1Example");

enum ThrottlingMode {
  NO_THROTTLING, //indica que não está em throttling no momento
  PHASE_1, //permite atender 3 requisições por segundo
  PHASE_2, //permite atender 1 requisição por segundo
  SLEEPING //não atende requisições
};

/* Modela o consumo de energia do sensor */
static Ptr<SensorEnergyModel> sensorEnergyModel;

/* Utilizado para controlar a frequencia com que os logs sobre consumo de energia do Wi-Fi
são exibidos */
static double segundoUltimoLogWifi = -1; 

/* Utilizado para controlar a frequencia com que os logs sobre energia total restante do nó
são exibidos */
static double segundoUltimoLogEnergiaRestante = -1;

/* Utilizado para controlar se uma requisição pode ser aceita ou não, baseado nas regras
de temporização quando em throttling */
static double segundoUltimaRequisicaoAceita = -1;

/* Capacidade de energia do nó, equivalente a 100% de bateria. */
static double maxEnergy = 1.0;

/* Modo atual de throttling em que o nó se encontra no momento. */
static ThrottlingMode throttlingMode = NO_THROTTLING;

std::string getThrottlingMode(){
    if (throttlingMode == NO_THROTTLING)
        return "No Throttling";
    else if (throttlingMode == PHASE_1)
        return "Phase 1";
    else if (throttlingMode == PHASE_2)
        return "Phase 2";
    else if (throttlingMode == ThrottlingMode::SLEEPING)
        return "Sleeping";
}

/**
 * Print a received packet
 *
 * \param from sender address
 * \return a string with the details of the packet: dst {IP, port}, time.
 */
static inline std::string
PrintReceivedPacket(Address& from)
{
    InetSocketAddress iaddr = InetSocketAddress::ConvertFrom(from);

    std::ostringstream oss;
    oss << "--\nReceived one packet! Socket: " << iaddr.GetIpv4() << " port: " << iaddr.GetPort()
        << " at time = " << Simulator::Now().GetSeconds() << "\n--";

    return oss.str();
}

/* Realiza a leitura pelo sensor e descarrega energia da bateria. */
static inline void readSensorValue(){
    NS_LOG_UNCOND(Simulator::Now().GetSeconds() << " Sensor realizando leitura...");
    sensorEnergyModel->SetCurrentA(0.75);
}

/* Responde a uma requisição de um cliente. */
static inline void respondRequest(Ptr<Socket> socket, Ptr<Packet> packet, Address from){
    NS_LOG_UNCOND(Simulator::Now().GetSeconds() << " Sensor respondendo requisição...");
    socket->SendTo(packet, 0, from);
}

void receiveSocket(Ptr<Socket> socket, bool accept){
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        if (packet->GetSize() > 0)
        {
            NS_LOG_UNCOND(PrintReceivedPacket(from));

            if (accept){
                
                NS_LOG_UNCOND("Requisição aceita");
                readSensorValue();
                respondRequest(socket, packet, from);

            } else {
                
                NS_LOG_UNCOND("Requisição negada");

                //requisição não será atendida, mas é necessário avisar o cliente
                //a negativa se dá com uma reposta menor do que a padrão 
                respondRequest(socket, Create<Packet>(8), from);
            }

        }
    }
}

/**
 * \param socket Pointer to socket.
 *
 * Packet receiving sink.
 * Este método é chamado quando o sensor recebe uma mensagem do cliente.
 */
void
ReceivePacket(Ptr<Socket> socket)
{
    if (throttlingMode == ThrottlingMode::SLEEPING) {
        
        //não atende nem responde a requisição
        NS_LOG_UNCOND("Requisição negada (modo sleep)");
        return;

    } else if (throttlingMode == PHASE_2){

        double segundoAtual = Simulator::Now().GetSeconds();
        bool mayReceivePacket = segundoAtual >= segundoUltimaRequisicaoAceita + 1;
        
        if (mayReceivePacket)
            segundoUltimaRequisicaoAceita = segundoAtual;

        receiveSocket(socket, mayReceivePacket);

    } else if (throttlingMode == PHASE_1) {

        double segundoAtual = Simulator::Now().GetSeconds();
        bool mayReceivePacket = segundoAtual >= segundoUltimaRequisicaoAceita + 0.333;
        
        if (mayReceivePacket)
            segundoUltimaRequisicaoAceita = segundoAtual;

        receiveSocket(socket, mayReceivePacket);

    } else if (throttlingMode == NO_THROTTLING){
        segundoUltimaRequisicaoAceita = Simulator::Now().GetSeconds();
        receiveSocket(socket, true);
    }
    
}

/**
 * \param socket Pointer to socket.
 * \param pktSize Packet size.
 * \param n Pointer to node.
 * \param pktCount Number of packets to generate.
 * \param pktInterval Packet sending interval.
 *
 * Traffic generator.
 */
static void
GenerateClientTraffic(Ptr<Socket> socket,
                uint32_t pktSize,
                Ptr<Node> n,
                uint32_t pktCount,
                Time pktInterval)
{
    if (pktCount > 0)
    {
        socket->Send(Create<Packet>(pktSize));
        Simulator::Schedule(pktInterval,
                            &GenerateClientTraffic,
                            socket,
                            pktSize,
                            n,
                            pktCount - 1,
                            pktInterval);
    }
    else
    {
        socket->Close();
    }
}

/**
 * Trace function for remaining energy at node.
 *
 * \param oldValue Old value
 * \param remainingEnergy New value
 */
void
RemainingEnergy(double oldValue, double remainingEnergy)
{
    double percentage = remainingEnergy / maxEnergy;
    
    if (percentage >= 0.7)
        throttlingMode = ThrottlingMode::NO_THROTTLING;
    else if (percentage >= 0.35 && percentage < 0.7)
        throttlingMode = ThrottlingMode::PHASE_1;
    else if (percentage >= 0.1 && percentage < 0.35)
        throttlingMode = ThrottlingMode::PHASE_2;
    else
        throttlingMode = ThrottlingMode::SLEEPING;

    double segundoAtual = Simulator::Now().GetSeconds();

    //Só exibe o log a cada 0.05 segundos
    if (segundoAtual >= segundoUltimoLogEnergiaRestante + 0.05){
        NS_LOG_UNCOND(Simulator::Now().GetSeconds()
                    << "s Current remaining energy = " << remainingEnergy << "J (" << getThrottlingMode() << ")");
        
        segundoUltimoLogEnergiaRestante = segundoAtual;
    }
}

/**
 * Trace function for total energy consumption at node.
 *
 * \param oldValue Old value
 * \param totalEnergy New value
 */
void
TotalWifiEnergy(double oldValue, double totalEnergy)
{
    double segundoAtual = Simulator::Now().GetSeconds();

    //Só exibe o log a cada 0.05 segundos
    if (segundoAtual >= segundoUltimoLogWifi + 0.05){
        NS_LOG_UNCOND(Simulator::Now().GetSeconds()
                  << "s Total energy consumed by wifi = " << totalEnergy << "J");

        segundoUltimoLogWifi = segundoAtual;
    }
}

/**
 * Trace function for total energy consumption at sensor.
 *
 * \param oldValue Old value
 * \param totalEnergy New value
 */
void
TotalSensorEnergy(double oldValue, double totalEnergy)
{
    NS_LOG_UNCOND(Simulator::Now().GetSeconds()
                  << "s Total energy consumed by sensor = " << totalEnergy << "J");
}

/**
 * Trace function for the power harvested by the energy harvester.
 *
 * \param oldValue Old value
 * \param harvestedPower New value
 */
void
HarvestedPower(double oldValue, double harvestedPower)
{
    /*NS_LOG_UNCOND(Simulator::Now().GetSeconds()
                  << "s Current harvested power = " << harvestedPower << " W");*/
}

/**
 * Trace function for the total energy harvested by the node.
 *
 * \param oldValue Old value
 * \param totalEnergyHarvested New value
 */
void
TotalEnergyHarvested(double oldValue, double totalEnergyHarvested)
{
    NS_LOG_UNCOND(Simulator::Now().GetSeconds()
                  << "s Total energy harvested by harvester = " << totalEnergyHarvested << " J");
}

int
main(int argc, char* argv[])
{
    std::string phyMode("DsssRate1Mbps");
    double Prss = -50;         // dBm (sinal excelente)
    uint32_t PacketSize = 200; // bytes
    bool verbose = false;

    // simulation parameters
    uint32_t numPackets = 100000; // number of packets to send
    double interval = 0.1;         // seconds
    double startTime = 0.0;      // seconds
    double distanceToRx = 10.0; // meters

    // Energy Harvester variables
    double harvestingUpdateInterval = 0.1; // seconds

    CommandLine cmd(__FILE__);
    cmd.AddValue("phyMode", "Wifi Phy mode", phyMode);
    cmd.AddValue("Prss", "Intended primary RSS (dBm)", Prss);
    cmd.AddValue("PacketSize", "size of application packet sent", PacketSize);
    cmd.AddValue("numPackets", "Total number of packets to send", numPackets);
    cmd.AddValue("startTime", "Simulation start time", startTime);
    cmd.AddValue("distanceToRx", "X-Axis distance between nodes", distanceToRx);
    cmd.AddValue("verbose", "Turn on all device log components", verbose);
    cmd.Parse(argc, argv);

    // Convert to time object
    Time interPacketInterval = Seconds(interval);

    // disable fragmentation for frames below 2200 bytes
    Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold",
                       StringValue("2200"));
    // turn off RTS/CTS for frames below 2200 bytes
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("2200"));
    // Fix non-unicast data rate to be the same as that of unicast
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));

    NodeContainer c;
    c.Create(2); // create 2 nodes
    NodeContainer networkNodes;
    networkNodes.Add(c.Get(0));
    networkNodes.Add(c.Get(1));

    Ptr<Node> clientNode = networkNodes.Get(0);
    Ptr<Node> sensorNode = networkNodes.Get(1);

    // The below set of helpers will help us to put together the wifi NICs we want
    WifiHelper wifi;
    if (verbose)
    {
        WifiHelper::EnableLogComponents();
    }
    wifi.SetStandard(WIFI_STANDARD_80211b);

    /** Wifi PHY **/
    /***************************************************************************/
    YansWifiPhyHelper wifiPhy;

    /** wifi channel **/
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");

    // create wifi channel
    Ptr<YansWifiChannel> wifiChannelPtr = wifiChannel.Create();
    wifiPhy.SetChannel(wifiChannelPtr);

    /** MAC layer **/
    // Add a MAC and disable rate control
    WifiMacHelper wifiMac;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue(phyMode),
                                 "ControlMode",
                                 StringValue(phyMode));
    // Set it to ad-hoc mode
    wifiMac.SetType("ns3::AdhocWifiMac");

    /** install PHY + MAC **/
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, networkNodes);

    // mobility (configurando posicionamento e mobilidade dos nós)
    // nós são estacionários e estão distantes um do outro conforme previamente estabelecido
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(2 * distanceToRx, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(c);

    /** Energy Model **/
    /***************************************************************************/
    /* energy source */
    BasicEnergySourceHelper basicSourceHelper;
    // configure energy source
    basicSourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(maxEnergy));
    // install energy source on node
    EnergySourceContainer energySources = basicSourceHelper.Install(sensorNode);
    
    /* wifi energy model */
    WifiRadioEnergyModelHelper wifiEnergyHelper;
    // configure wifi energy model
    wifiEnergyHelper.Set("TxCurrentA", DoubleValue(0.0174));
    wifiEnergyHelper.Set("RxCurrentA", DoubleValue(0.0197));
    // install energy model on net devices
    DeviceEnergyModelContainer devEnergyModelsContainer = wifiEnergyHelper.Install(devices.Get(1), energySources);

    /* sensor energy model */
    sensorEnergyModel = CreateObject<SensorEnergyModel>();
    sensorEnergyModel->SetEnergySource(energySources.Get(0));
    energySources.Get(0)->AppendDeviceEnergyModel(sensorEnergyModel);
    sensorEnergyModel->SetNode(sensorNode);
    sensorEnergyModel->SetCurrentA(0);

    /* energy harvester */
    BasicEnergyHarvesterHelper basicHarvesterHelper;
    // configure energy harvester
    basicHarvesterHelper.Set("PeriodicHarvestedPowerUpdateInterval",
                             TimeValue(Seconds(harvestingUpdateInterval)));
    basicHarvesterHelper.Set("HarvestablePower",
                             StringValue("ns3::UniformRandomVariable[Min=0.0|Max=0.1]"));
    // install harvester on all energy sources
    EnergyHarvesterContainer harvesters = basicHarvesterHelper.Install(energySources);
    /***************************************************************************/

    /** Internet stack **/
    InternetStackHelper internet;
    internet.Install(networkNodes);

    Ipv4AddressHelper ipv4;
    NS_LOG_INFO("Assign IP Addresses.");
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i = ipv4.Assign(devices);

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> recvSink = Socket::CreateSocket(sensorNode, tid); // node 1, sensor
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 80);
    recvSink->Bind(local);
    recvSink->SetRecvCallback(MakeCallback(&ReceivePacket));

    //socket a ser enviado do cliente para o sensor
    Ptr<Socket> socket = Socket::CreateSocket(clientNode, tid); // node 0, client
    InetSocketAddress remote = InetSocketAddress(Ipv4Address::GetBroadcast(), 80);
    socket->SetAllowBroadcast(true);
    socket->Connect(remote);

    /** connect trace sources **/
    /***************************************************************************/
    // all traces are connected to node 1 (sensor)
    // energy source
    Ptr<BasicEnergySource> basicSourcePtr = DynamicCast<BasicEnergySource>(energySources.Get(0));
    basicSourcePtr->TraceConnectWithoutContext("RemainingEnergy", MakeCallback(&RemainingEnergy));
    // wifi energy model
    Ptr<DeviceEnergyModel> basicRadioModelPtr =
        basicSourcePtr->FindDeviceEnergyModels("ns3::WifiRadioEnergyModel").Get(0);
    NS_ASSERT(basicRadioModelPtr);
    basicRadioModelPtr->TraceConnectWithoutContext("TotalEnergyConsumption",
                                                   MakeCallback(&TotalWifiEnergy));

     // sensor energy model
    Ptr<DeviceEnergyModel> sensorModelPtr =
        basicSourcePtr->FindDeviceEnergyModels("ns3::SensorEnergyModel").Get(0);
    NS_ASSERT(sensorModelPtr);
    sensorModelPtr->TraceConnectWithoutContext("TotalEnergyConsumption",
                                                   MakeCallback(&TotalSensorEnergy));

    // energy harvester
    Ptr<BasicEnergyHarvester> basicHarvesterPtr =
        DynamicCast<BasicEnergyHarvester>(harvesters.Get(0));
    basicHarvesterPtr->TraceConnectWithoutContext("HarvestedPower", MakeCallback(&HarvestedPower));
    basicHarvesterPtr->TraceConnectWithoutContext("TotalEnergyHarvested",
                                                  MakeCallback(&TotalEnergyHarvested));
    /***************************************************************************/

    /** simulation setup **/
    // start traffic
    Simulator::Schedule(Seconds(startTime),
                        &GenerateClientTraffic,
                        socket,
                        PacketSize,
                        networkNodes.Get(0),
                        numPackets,
                        interPacketInterval);

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    for (auto iter = devEnergyModelsContainer.Begin(); iter != devEnergyModelsContainer.End(); iter++)
    {
        double energyConsumed = (*iter)->GetTotalEnergyConsumption();
        NS_LOG_UNCOND("End of simulation ("
                      << Simulator::Now().GetSeconds()
                      << "s) Total energy consumed by radio = " << energyConsumed << "J");
        NS_ASSERT(energyConsumed <= 1.0);
    }

    Simulator::Destroy();

    return 0;
}
