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

#include "ecs-node-app.h"

#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/node-depleted-exception.h"
#include "ns3/node-asleep-exception.h"
#include "ns3/log-utils.h"
#include "ns3/utils.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ECSNodeApp");
NS_OBJECT_ENSURE_REGISTERED(ECSNodeApp);

TypeId
ECSNodeApp::GetTypeId()
{
    NS_LOG_FUNCTION("ECSNodeApp::GetTypeId()");

    static TypeId tid =
        TypeId("ns3::ECSNodeApp")
            .SetParent<CheckpointApp>()
            .SetGroupName("Applications")
            .AddConstructor<ECSNodeApp>()
            .AddAttribute("Port",
                          "Port on which we listen for incoming packets.",
                          UintegerValue(9),
                          MakeUintegerAccessor(&ECSNodeApp::m_port),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("NodeName",
                            "This node's name. ",
                            StringValue(""),
                            MakeStringAccessor(&ECSNodeApp::nodeName),
                            MakeStringChecker())
            .AddAttribute("GlobalSeq",
                          "Numeração de sequência global das mensagens recebidas. É incrementada ao receber uma mensagem.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&ECSNodeApp::global_seq),
                          MakeUintegerChecker<uint64_t>())
            .AddAttribute("ConfigFilename",
                            "This node's config filename. ",
                            StringValue(""),
                            MakeStringAccessor(&ECSNodeApp::configFilename),
                            MakeStringChecker())
            .AddAttribute("TotalNodesQuantity",
                            "Quantidade total de nós do sistema. ",
                            UintegerValue(0),
                            MakeIntegerAccessor(&ECSNodeApp::totalNodesQuantity),
                            MakeIntegerChecker<int>());
    
    return tid;
}

ECSNodeApp::ECSNodeApp()
    : global_seq(0){
    NS_LOG_FUNCTION(this);

    currentMode = NORMAL;
    udpHelper = Create<UDPHelper>();
    applicationType = CSS;
}

ECSNodeApp::~ECSNodeApp()
{
    NS_LOG_FUNCTION(this);
    udpHelper = nullptr;
}

void
ECSNodeApp::StartApplication(){
    NS_LOG_FUNCTION(this);

    if (!configHelper){
        configHelper = Create<ConfigHelper>(configFilename);
    }

    loadConfigurations();

    if (udpHelper->isDisconnected())
        udpHelper->configureServer(GetNode(), getNodeName(), m_port);

    //Atribuindo um callback de recebimento e envio de mensagens para o protocolo de checkpointing e para a aplicação
    udpHelper->setProtocolSendCallback(MakeCallback(&CheckpointStrategy::interceptSend, Ptr<CheckpointStrategy>(checkpointStrategy)));
    udpHelper->setProtocolReceiveCallback(MakeCallback(&CheckpointStrategy::interceptRead, Ptr<CheckpointStrategy>(checkpointStrategy)));
    setProtocolAfterReceiveCallback(MakeCallback(&CheckpointStrategy::afterMessageReceive, Ptr<CheckpointStrategy>(checkpointStrategy)));
    udpHelper->setReceiveCallback(MakeCallback(&ECSNodeApp::HandleRead, Ptr<ECSNodeApp>(this)));

    NS_LOG_INFO("Iniciando " << getNodeName() << "...");
    
    //Obtendo IP do nó
    Ptr<Node> node = GetNode();
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();

    // Geralmente a interface 0 é loopback, então o IP "real" começa na 1
    Ipv4InterfaceAddress iaddr = ipv4->GetAddress(1, 0);
    Ipv4Address ipAddr = iaddr.GetLocal();

    NS_LOG_INFO(getNodeName() << " iniciado com IP " << ipAddr << ".");
    
    decreaseConnectEnergy();
}

void ECSNodeApp::StopApplication() {
    NS_LOG_FUNCTION(this);
    udpHelper->terminateConnection();
}

void ECSNodeApp::HandleRead(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this << md);

    utils::logMessageReceived(getNodeName(), md);

    global_seq++;
    protocolAfterReceiveCallback(md);

    if (md->GetCommand() == REQUEST_VALUE || md->GetCommand() == RESPONSE_VALUE){
        //Encaminhando a mensagem para o destinatário final
        Ptr<MessageData> mdResp = forwardMessageToFinalDestination(md);
    }

}

Ptr<MessageData> ECSNodeApp::forwardMessageToFinalDestination(Ptr<MessageData> md){
    
    Ipv4Address finalDestinationIP = utils::convertStringToIpv4Address(md->getPiggyBackedValue("finalDestinationIP"));
    uint16_t finalDestinationPort = static_cast<uint16_t>(stoi(md->getPiggyBackedValue("finalDestinationPort")));
    
    Ipv4Address originalSenderIP = utils::convertAddressToIpv4Address(md->GetFrom());
    md->SetPiggyBackedInfo("originalSenderIP " + utils::convertIpv4AddressToString(originalSenderIP));

    return send(md->GetCommand(), md->GetData(), finalDestinationIP, finalDestinationPort, false, md->GetPiggyBackedInfo());

}

json ECSNodeApp::to_json() const {
    NS_LOG_FUNCTION(this);

    json j = ECSCheckpointApp::to_json();
    j["m_port"] = m_port;
    j["global_seq"] = global_seq;

    return j;
}

void ECSNodeApp::from_json(const json& j) {
    NS_LOG_FUNCTION(this);
    
    ECSCheckpointApp::from_json(j);

    j.at("m_port").get_to(m_port);
    j.at("global_seq").get_to(global_seq); 
}

} // Namespace ns3
