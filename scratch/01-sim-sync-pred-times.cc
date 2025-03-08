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
#include <fstream>
#include <iostream>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("myExample");

int
main(int argc, char* argv[])
{
    //Habilitando mensagens de log
	
    
    LogComponentEnable("Application", LOG_INFO);
    LogComponentEnable("CheckpointApp", LOG_INFO);
    LogComponentEnable("BatteryServerApp", LOG_INFO);
    LogComponentEnable("ClientApp", LOG_INFO);
    LogComponentEnable("Battery", LOG_INFO);
    LogComponentEnable("CheckpointStrategy", LOG_INFO);
    LogComponentEnable("CheckpointHelper", LOG_INFO);
    LogComponentEnable("SyncPredefinedTimesCheckpoint", LOG_INFO);
    
    // LogComponentEnable("Application", LOG_FUNCTION);
    // LogComponentEnable("CheckpointApp", LOG_FUNCTION);
    // LogComponentEnable("BatteryServerApp", LOG_FUNCTION);
    // LogComponentEnable("ClientApp", LOG_FUNCTION);
    // LogComponentEnable("Battery", LOG_FUNCTION);
    // LogComponentEnable("CheckpointStrategy", LOG_FUNCTION);
    // LogComponentEnable("CheckpointHelper", LOG_FUNCTION);
    // LogComponentEnable("SyncPredefinedTimesCheckpoint", LOG_FUNCTION);
    
    // LogComponentEnable("Application", LOG_LEVEL_ALL);
    // LogComponentEnable("CheckpointApp", LOG_LEVEL_ALL);
    // LogComponentEnable("BatteryServerApp", LOG_LEVEL_ALL);
    // LogComponentEnable("ClientApp", LOG_LEVEL_ALL);
    // LogComponentEnable("Battery", LOG_LEVEL_ALL);
    // LogComponentEnable("CheckpointStrategy", LOG_LEVEL_ALL);
    // LogComponentEnable("CheckpointHelper", LOG_LEVEL_ALL);
    // LogComponentEnable("SyncPredefinedTimesCheckpoint", LOG_LEVEL_ALL);
    // LogComponentEnable("EnergyGenerator", LOG_LEVEL_ALL);
    // LogComponentEnable("CircularEnergyGenerator", LOG_LEVEL_ALL);

    //Habilitando a impressão de pacotes
    ns3::PacketMetadata::Enable();

    // simulation parameters
    double distanceToRx = 10.0; // meters
    UintegerValue maxPackets = UintegerValue(0); //número máximo de pacotes que podem ser enviados na simulação (0 = ilimitado).
    TimeValue interval = TimeValue(Seconds(1.0)); //tempo que deve ser aguardado entre envio de pacotes
    UintegerValue packetSize = UintegerValue(1024);

    std::string phyMode("DsssRate1Mbps");
    bool verbose = false;

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
    Ptr<Node> batteryServerNode = networkNodes.Get(1);

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

    /** Internet stack **/
    InternetStackHelper internet;
    internet.Install(networkNodes);

    Ipv4AddressHelper ipv4;
    NS_LOG_INFO("Assign IP Addresses.");
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i = ipv4.Assign(devices);

    //Removendo checkpoints criados em simulações anteriores
    CheckpointHelper ch;
    ch.removeAllCheckpointsAndLogs();

    //Cria uma aplicação de servidor a bateria, de forma a gerar tráfego de dados. 9 é a porta.
    BatteryNodeAppHelper batteryServerApp(9); 

    ApplicationContainer serverApps = batteryServerApp.Install(batteryServerNode); //define o nó 1 como servidor
    serverApps.Start(Seconds(1.0)); //o servidor será iniciado com 1s de simulação

    ClientNodeAppHelper clientApp(i.GetAddress(1), 9); //cria uma aplicação cliente que irá enviar pacotes para a porta 9 do endereço do servidor
    clientApp.SetAttribute("MaxPackets", maxPackets); //número máximo de pacotes que podem ser enviados na simulação.
    clientApp.SetAttribute("Interval", interval); //tempo que deve ser aguardado entre envio de pacotes
    clientApp.SetAttribute("PacketSize", packetSize); //tamanho dos pacotes

    ApplicationContainer clientApps = clientApp.Install(clientNode); //instala a aplicação cliente no nó 0
    clientApps.Start(Seconds(2.0)); //inicia o cliente com 2s de simulação

    /** simulation setup **/

    Simulator::Stop(Seconds(60.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
