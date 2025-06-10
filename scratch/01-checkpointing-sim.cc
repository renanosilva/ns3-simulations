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
 * Exemplo de utilização de servidores com consumo de bateria (ex.: sensores) 
 * e geração de energia simulados (autoria própria).
 * 
 * @author Renan
 */

#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/config-helper.h"
#include <fstream>
#include <vector>

using namespace ns3;
using namespace utils;

NS_LOG_COMPONENT_DEFINE("Sim01");

/** Nome do arquivo de configuração da simulação */
static const string CONFIG_FILENAME = "scratch/03-efficient-assync-recovery.json"; 

int
main(int argc, char* argv[])
{

    //Habilitando mensagens de log
    
    LogComponentEnable("Application", LOG_INFO);
    LogComponentEnable("CheckpointApp", LOG_INFO);
    LogComponentEnable("ServerNodeApp", LOG_INFO);
    LogComponentEnable("ClientApp", LOG_INFO);
    LogComponentEnable("Battery", LOG_INFO);
    LogComponentEnable("CheckpointStrategy", LOG_INFO);
    LogComponentEnable("CheckpointHelper", LOG_INFO);
    LogComponentEnable("GlobalSyncClocksStrategy", LOG_INFO);
    LogComponentEnable("DecentralizedRecoveryProtocol", LOG_INFO);
    LogComponentEnable("EfficientAssyncRecoveryProtocol", LOG_INFO);
    LogComponentEnable("EnergyGenerator", LOG_INFO);
    LogComponentEnable("CircularEnergyGenerator", LOG_INFO);
    LogComponentEnable("UDPHelper", LOG_INFO);
    LogComponentEnable("LogUtils", LOG_INFO);

    // LogComponentEnable("Application", LOG_INFO);
    // LogComponentEnable("CheckpointApp", LOG_ALL);
    // LogComponentEnable("ServerNodeApp", LOG_ALL);
    // LogComponentEnable("ClientApp", LOG_ALL);
    // LogComponentEnable("Battery", LOG_INFO);
    // LogComponentEnable("CheckpointStrategy", LOG_ALL);
    // LogComponentEnable("CheckpointHelper", LOG_ALL);
    // LogComponentEnable("GlobalSyncClocksStrategy", LOG_ALL);
    // LogComponentEnable("DecentralizedRecoveryProtocol", LOG_ALL);
    // LogComponentEnable("EfficientAssyncRecoveryProtocol", LOG_ALL);
    // LogComponentEnable("EnergyGenerator", LOG_INFO);
    // LogComponentEnable("CircularEnergyGenerator", LOG_INFO);
    // LogComponentEnable("UDPHelper", LOG_ALL);
    // LogComponentEnable("LogUtils", LOG_INFO);

    // LogComponentEnable("Application", LOG_FUNCTION);
    // LogComponentEnable("CheckpointApp", LOG_FUNCTION);
    // LogComponentEnable("BatteryNodeApp", LOG_FUNCTION);
    // LogComponentEnable("ClientApp", LOG_FUNCTION);
    // LogComponentEnable("Battery", LOG_FUNCTION);
    // LogComponentEnable("CheckpointStrategy", LOG_FUNCTION);
    // LogComponentEnable("CheckpointHelper", LOG_FUNCTION);
    // LogComponentEnable("GlobalSyncClocksStrategy", LOG_FUNCTION);
    // LogComponentEnable("DecentralizedRecoveryProtocol", LOG_FUNCTION);
    // LogComponentEnable("EfficientAssyncRecoveryProtocol", LOG_FUNCTION);
    // LogComponentEnable("EnergyGenerator", LOG_FUNCTION);
    // LogComponentEnable("CircularEnergyGenerator", LOG_FUNCTION);
    // LogComponentEnable("UDPHelper", LOG_FUNCTION);
    
    // LogComponentEnable("Application", LOG_LEVEL_ALL);
    // LogComponentEnable("CheckpointApp", LOG_LEVEL_ALL);
    // LogComponentEnable("ServerNodeApp", LOG_LEVEL_ALL);
    // LogComponentEnable("ClientApp", LOG_LEVEL_ALL);
    // LogComponentEnable("Battery", LOG_LEVEL_ALL);
    // LogComponentEnable("CheckpointStrategy", LOG_LEVEL_ALL);
    // LogComponentEnable("CheckpointHelper", LOG_LEVEL_ALL);
    // LogComponentEnable("GlobalSyncClocksStrategy", LOG_LEVEL_ALL);
    // LogComponentEnable("DecentralizedRecoveryProtocol", LOG_LEVEL_ALL);
    // LogComponentEnable("EfficientAssyncRecoveryProtocol", LOG_LEVEL_ALL);
    // LogComponentEnable("EnergyGenerator", LOG_LEVEL_ALL);
    // LogComponentEnable("CircularEnergyGenerator", LOG_LEVEL_ALL);
    // LogComponentEnable("UDPHelper", LOG_LEVEL_ALL);
    // LogComponentEnable("LogUtils", LOG_LEVEL_ALL);

    //Habilitando a impressão de pacotes
    ns3::PacketMetadata::Enable();

    ConfigHelper config = ConfigHelper(CONFIG_FILENAME);
    double simulationTime = config.GetDoubleProperty("simulation.time-in-seconds");
    double distanceToRx = config.GetDoubleProperty("simulation.distance-to-rx-in-meters");
    UintegerValue maxPackets = UintegerValue(config.GetIntProperty("simulation.max-packets"));
    TimeValue interval = TimeValue(Seconds(config.GetDoubleProperty("simulation.interval")));
    vector<string> serverNodesNames = config.GetNodesWithRole("server");
    vector<string> clientNodesNames = config.GetNodesWithRole("client");
    int serverNodesQuantity = serverNodesNames.size();
    int clientNodesQuantity = clientNodesNames.size();
    int totalNodesQuantity = serverNodesQuantity + clientNodesQuantity;
    
    std::string phyMode("DsssRate1Mbps");
    bool verbose = false;

    // disable fragmentation for frames below 2200 bytes
    Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold",
                       StringValue("2200"));
    // turn off RTS/CTS for frames below 2200 bytes
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("2200"));
    // Fix non-unicast data rate to be the same as that of unicast
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));
    
    //criando nós
    NodeContainer nodes;
    nodes.Create(totalNodesQuantity);

    NodeContainer clientNodes;
    NodeContainer serverNodes;

    //Os primeiros nós registrados serão os servidores
    for (int i = 0; i < serverNodesQuantity; i++){
        serverNodes.Add(nodes.Get(i));
    }

    //Os demais serão os clientes
    for (int i = serverNodesQuantity; i < totalNodesQuantity; i++){
        clientNodes.Add(nodes.Get(i));
    }

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
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);

    // mobility (configurando posicionamento e mobilidade dos nós)
    // nós são estacionários e estão distantes um do outro conforme previamente estabelecido
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(2 * distanceToRx, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    /** Internet stack **/
    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper ipv4;
    NS_LOG_INFO("Assign IP Addresses.");
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i = ipv4.Assign(devices);

    //Removendo checkpoints criados em simulações anteriores
    CheckpointHelper ch;
    ch.removeAllCheckpointsAndLogs();

    vector<Ipv4Address> serverAddresses;

    //Iniciando servidores
    for (int j = 0; j < serverNodesQuantity; j++){
        
        //Cria uma aplicação de servidor, de forma a gerar tráfego de dados. 9 é a porta.
        ServerNodeAppHelper serverAppHelper(9, serverNodesNames[j], CONFIG_FILENAME, totalNodesQuantity);
        
        //Instala a aplicação no nó i e agenda sua inicialização
        serverAppHelper.Install(serverNodes.Get(j)).Start(Seconds(1.0));
        serverAddresses.push_back(i.GetAddress(j));
    }

    //Iniciando aplicações clientes
    for (int j = 0; j < clientNodesQuantity; j++){
        
        //cria uma aplicação cliente que irá enviar pacotes para a porta 9 do endereço do servidor
        ClientNodeAppHelper clientAppHelper(serverAddresses, 9, clientNodesNames[j], CONFIG_FILENAME, totalNodesQuantity);
        clientAppHelper.SetAttribute("MaxPackets", maxPackets); //número máximo de pacotes que podem ser enviados na simulação.
        clientAppHelper.SetAttribute("Interval", interval); //tempo que deve ser aguardado entre envio de pacotes

        clientAppHelper.Install(clientNodes.Get(j)).Start(Seconds(1.0)); //instala a aplicação cliente no nó e agenda sua inicialização
    }

    /** simulation setup **/

    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
