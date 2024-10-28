/*
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

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1
//    point-to-point
//

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FirstScriptExample");

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS); //resolução de tempo padrão é de nanosegundos
    
	//Permite que mensagens sejam exibidas durante envio e recebimento de pacotes (para clientes e servidores)
	LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    NodeContainer nodes;
    nodes.Create(2); //cria dois nós e guarda referências a eles

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices; //placas de rede ou dispositivos de rede
    devices = pointToPoint.Install(nodes); //instala a placa de rede nos nós

    InternetStackHelper stack;
    stack.Install(nodes); //instala protocolos de rede (TCP, UDP, IP) nos nós

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0"); //define o padrão de alocação de IPs

    Ipv4InterfaceContainer interfaces = address.Assign(devices); //atribui IPs aos dispositivos, de acordo com o padrão definido

    UdpEchoServerHelper echoServer(9); //Cria uma aplicação de servidor, de forma a gerar tráfego de dados. 9 é a porta.

    ApplicationContainer serverApps = echoServer.Install(nodes.Get(1)); //define o nó 1 como servidor
    serverApps.Start(Seconds(1.0)); //o servidor será iniciado com 1s de simulação
    serverApps.Stop(Seconds(10.0)); //o servidor será finalizado com 1s de simulação

    UdpEchoClientHelper echoClient(interfaces.GetAddress(1), 9); //cria uma aplicação cliente que irá enviar pacotes para a porta 9 do endereço do servidor
    echoClient.SetAttribute("MaxPackets", UintegerValue(1)); //número máximo de pacotes que podem ser enviados na simulação.
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0))); //tempo que deve ser aguardado entre envio de pacotes
    echoClient.SetAttribute("PacketSize", UintegerValue(1024)); //tamanho dos pacotes

    ApplicationContainer clientApps = echoClient.Install(nodes.Get(0)); //instala a aplicação cliente no nó 0
    clientApps.Start(Seconds(2.0)); //inicia o cliente com 2s de simulação
    clientApps.Stop(Seconds(10.0)); //para o cliente com 10s de simulação

	//Simulação continua até que não existam mais eventos agendados ou então até que o comando Simulator::Stop seja executado
    Simulator::Run();
    Simulator::Destroy(); 
    return 0;
}
