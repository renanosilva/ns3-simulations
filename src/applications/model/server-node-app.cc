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

#include "server-node-app.h"

#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/node-depleted-exception.h"
#include "ns3/node-asleep-exception.h"
#include "ns3/log-utils.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ServerNodeApp");
NS_OBJECT_ENSURE_REGISTERED(ServerNodeApp);

TypeId
ServerNodeApp::GetTypeId()
{
    NS_LOG_FUNCTION("ServerNodeApp::GetTypeId()");

    static TypeId tid =
        TypeId("ns3::ServerNodeApp")
            .SetParent<CheckpointApp>()
            .SetGroupName("Applications")
            .AddConstructor<ServerNodeApp>()
            .AddAttribute("Port",
                          "Port on which we listen for incoming packets.",
                          UintegerValue(9),
                          MakeUintegerAccessor(&ServerNodeApp::m_port),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("NodeName",
                            "This node's name. ",
                            StringValue(""),
                            MakeStringAccessor(&ServerNodeApp::nodeName),
                            MakeStringChecker())
            .AddAttribute("Seq",
                          "Numeração de sequência das mensagens recebidas. É incrementada ao receber uma mensagem.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&ServerNodeApp::m_seq),
                          MakeUintegerChecker<uint64_t>())
            .AddAttribute("ConfigFilename",
                            "This node's config filename. ",
                            StringValue(""),
                            MakeStringAccessor(&ServerNodeApp::configFilename),
                            MakeStringChecker())
            .AddAttribute("TotalNodesQuantity",
                            "Quantidade total de nós do sistema. ",
                            UintegerValue(0),
                            MakeIntegerAccessor(&ServerNodeApp::totalNodesQuantity),
                            MakeIntegerChecker<int>())
            .AddTraceSource("Rx",
                            "A packet has been received",
                            MakeTraceSourceAccessor(&ServerNodeApp::m_rxTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("RxWithAddresses",
                            "A packet has been received",
                            MakeTraceSourceAccessor(&ServerNodeApp::m_rxTraceWithAddresses),
                            "ns3::Packet::TwoAddressTracedCallback");
    
    return tid;
}

ServerNodeApp::ServerNodeApp()
    : m_seq(0){
    NS_LOG_FUNCTION(this);

    currentMode = NORMAL;
    udpHelper = Create<UDPHelper>();
    applicationType = SERVER;
}

ServerNodeApp::~ServerNodeApp()
{
    NS_LOG_FUNCTION(this);
    udpHelper = nullptr;
}

void
ServerNodeApp::StartApplication(){
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
    udpHelper->setReceiveCallback(MakeCallback(&ServerNodeApp::HandleRead, Ptr<ServerNodeApp>(this)));

    NS_LOG_INFO("Iniciando " << getNodeName() << "..." 
                << (battery == nullptr ? "" : "Energia inicial: " + to_string(battery->getRemainingEnergy()) + "."));


    NS_LOG_INFO(getNodeName() << " iniciado.");
    decreaseConnectEnergy();
}

void ServerNodeApp::StopApplication() {
    NS_LOG_FUNCTION(this);
    udpHelper->terminateConnection();
}

void ServerNodeApp::HandleRead(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this << md);

    if (isSleeping() || isDepleted()){
        //ignora mensagens se estiver em modo sleep ou descarregado
        return;
    }

    try {
        utils::logMessageReceived(getNodeName(), md);
        decreaseReadEnergy();

        protocolAfterReceiveCallback(md);

        if (md->GetCommand() == REQUEST_VALUE){
            //Respondendo a requisição
            m_seq++;
            Ptr<MessageData> mdResp = send(RESPONSE_VALUE, m_seq, md->GetFrom());
        }

    } catch (NodeAsleepException& e) {
        NS_LOG_LOGIC("Tarefa incompleta por estar em modo SLEEP.");
        return;
    } catch (NodeDepletedException& e) {
        NS_LOG_LOGIC("Tarefa incompleta por estar em modo DEPLETED.");
        return;
    }
}

void ServerNodeApp::replayReceive(Ptr<MessageData> md, bool replayResponse) {
    if (isSleeping() || isDepleted()){
        //ignora mensagens se estiver em modo sleep ou descarregado
        return;
    }

    utils::logMessageReceived(getNodeName(), md, true);
    
    if (md->GetCommand().find(REQUEST_VALUE) != string::npos){
        m_seq++;

        if (replayResponse){
            //Simulando envio de resposta
            Ptr<MessageData> mdResp = send(REPLAY_RESPONSE_VALUE, m_seq, md->GetFrom(), true);
        }
    }

    udpHelper->replayReceive(md);
}

void ServerNodeApp::resetNodeData() {
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO("\nAntes do rollback...");
    printNodeData();

    StopApplication();
    
    m_port = 0;
    m_seq = 0;
    udpHelper->DisposeReferences();
    udpHelper = nullptr;
    protocolAfterReceiveCallback.Nullify();
    checkpointStrategy->DisposeReferences();
    checkpointStrategy = nullptr;
}

void ServerNodeApp::printNodeData(){
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO("Dados de " << getNodeName() << ":" );
    NS_LOG_INFO(
        "currentMode = " << currentMode
        << (battery == nullptr ? "" : ", battery.getBatteryPercentage() = " + to_string(battery->getBatteryPercentage()))
        << (energyGenerator == nullptr ?   
            "" : ", energyGenerator->GetInstanceTypeId().GetName() = " + energyGenerator->GetTypeId().GetName())
        << ", idleEnergyConsumption = " << idleEnergyConsumption
        << ", sleepEnergyConsumption = " << sleepEnergyConsumption
        << ", energyUpdateInterval = " << energyUpdateInterval.As(Time::S)
        << ", rollbackInProgress = " << checkpointStrategy->isRollbackInProgress()
        << ", checkpointInProgress = " << checkpointStrategy->isCheckpointInProgress()
        << ", m_port = " << m_port
        << ", m_seq = " << m_seq
        << ", dependentAddresses.size() = " << checkpointStrategy->getDependentAddresses().size()
        << ", configFilename = " << configFilename);
    
    udpHelper->printData();
    checkpointStrategy->printData();
}

json ServerNodeApp::to_json() const {
    NS_LOG_FUNCTION(this);

    json j = CheckpointApp::to_json();
    j["m_port"] = m_port;
    j["m_seq"] = m_seq;

    return j;
}

void ServerNodeApp::from_json(const json& j) {
    NS_LOG_FUNCTION(this);
    
    CheckpointApp::from_json(j);

    j.at("m_port").get_to(m_port);
    j.at("m_seq").get_to(m_seq); 
}

} // Namespace ns3
