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
#include "ns3/log-utils.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BatteryNodeApp");

NS_OBJECT_ENSURE_REGISTERED(BatteryNodeApp);

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
            .AddAttribute("NodeName",
                            "This node's name. ",
                            StringValue(""),
                            MakeStringAccessor(&BatteryNodeApp::nodeName),
                            MakeStringChecker())
            .AddAttribute("Seq",
                          "Numeração de sequência das mensagens recebidas. É incrementada ao receber uma mensagem.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&BatteryNodeApp::m_seq),
                          MakeUintegerChecker<uint64_t>())
            .AddAttribute("ConfigFilename",
                            "This node's config filename. ",
                            StringValue(""),
                            MakeStringAccessor(&BatteryNodeApp::configFilename),
                            MakeStringChecker())
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
    : m_seq(0){
    NS_LOG_FUNCTION(this);

    currentMode = NORMAL;
    udpHelper = Create<UDPHelper>();
    applicationType = SERVER;
}

BatteryNodeApp::~BatteryNodeApp()
{
    NS_LOG_FUNCTION(this);
    udpHelper = nullptr;
}

void
BatteryNodeApp::StartApplication()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Iniciando " << getNodeName() << "... Energia inicial: " << battery.getRemainingEnergy() << ".");

    if (!configHelper)
        configHelper = Create<ConfigHelper>(configFilename);

    loadConfigurations();

    if (udpHelper->isDisconnected())
        udpHelper->configureServer(GetNode(), getNodeName(), m_port);

    //Atribuindo um callback de recebimento e envio de mensagens para o protocolo de checkpointing e para a aplicação
    udpHelper->setProtocolSendCallback(MakeCallback(&CheckpointStrategy::interceptSend, checkpointStrategy));
    udpHelper->setProtocolReceiveCallback(MakeCallback(&CheckpointStrategy::interceptRead, checkpointStrategy));
    udpHelper->setReceiveCallback(MakeCallback(&BatteryNodeApp::HandleRead, this));

    NS_LOG_INFO(getNodeName() << " conectado.");
    decreaseConnectEnergy();
}

void BatteryNodeApp::StopApplication() {
    NS_LOG_FUNCTION(this);
    udpHelper->terminateConnection();
}

void BatteryNodeApp::loadConfigurations() {
    NS_LOG_FUNCTION(this);

    string propertyPrefix = "nodes.battery-nodes." + getNodeName() + ".";
    
    energyUpdateInterval = Seconds(configHelper->GetDoubleProperty(propertyPrefix + "energy-update-interval"));
    sleepEnergyConsumption = configHelper->GetDoubleProperty(propertyPrefix + "sleep-energy");
    idleEnergyConsumption = configHelper->GetDoubleProperty(propertyPrefix + "idle-energy");
    rollbackEnergyConsumption = configHelper->GetDoubleProperty(propertyPrefix + "rollback-energy");
    createCheckpointConsumption = configHelper->GetDoubleProperty(propertyPrefix + "create-checkpoint-energy");
    receivePacketConsumption = configHelper->GetDoubleProperty(propertyPrefix + "receive-packet-energy");
    sendPacketConsumption = configHelper->GetDoubleProperty(propertyPrefix + "send-packet-energy");
    connectConsumption = configHelper->GetDoubleProperty(propertyPrefix + "connect-energy");
    sleepModePercentage = configHelper->GetDoubleProperty(propertyPrefix + "sleep-mode-percentage", 10.0);
    normalModePercentage = configHelper->GetDoubleProperty(propertyPrefix + "normal-mode-percentage", 20.0);
    
    if (!energyGenerator)
        configureEnergyGenerator();

    if (!checkpointStrategy)
        configureCheckpointStrategy();
}

void BatteryNodeApp::configureEnergyGenerator() {
    NS_LOG_FUNCTION(this);

    string property = "nodes.battery-nodes." + getNodeName() + ".energy-generator";
    string energyGeneratorName = configHelper->GetStringProperty(property);

    if (energyGeneratorName == "FixedEnergyGenerator"){
        
        string valueProperty = "nodes.battery-nodes." + getNodeName() + ".energy-fixed-value";
        double value = configHelper->GetDoubleProperty(valueProperty);

        energyGenerator = Create<FixedEnergyGenerator>(value);    

    } else if (energyGeneratorName == "CircularEnergyGenerator") {
        
        energyGenerator = Create<CircularEnergyGenerator>();

    } else {
        NS_ABORT_MSG("Não foi possível identificar a estratégia de geração de energia de " << getNodeName());
    }
    
    //Agendando geração de energia
    Simulator::Schedule(energyUpdateInterval,
            &BatteryNodeApp::generateEnergy,
            this);

    //Agendando desconto contínuo de energia referente ao funcionamento básico do dispositivo
    Simulator::Schedule(energyUpdateInterval,
            &BatteryNodeApp::decreaseCurrentModeEnergy,
            this);

}

void BatteryNodeApp::HandleRead(Ptr<MessageData> md){
    NS_LOG_FUNCTION(this << md);

    if (isSleeping() || isDepleted()){
        //ignora mensagens se estiver em modo sleep ou descarregado
        return;
    }

    try {
        utils::logMessageReceived(getNodeName(), md);

        decreaseReadEnergy();

        if (md->GetCommand() == REQUEST_VALUE){
            NS_LOG_LOGIC("Responding packet");

            m_seq++;
        
            Ptr<MessageData> mdResp = send(RESPONSE_VALUE, m_seq, md->GetFrom());
            decreaseSendEnergy();
        }

    } catch (NodeAsleepException& e) {
        NS_LOG_LOGIC("Tarefa incompleta por estar em modo SLEEP.");
        return;
    } catch (NodeDepletedException& e) {
        NS_LOG_LOGIC("Tarefa incompleta por estar em modo DEPLETED.");
        return;
    }
}

void BatteryNodeApp::initiateRollback(Address requester, int cpId){
    NS_LOG_FUNCTION(this);

    resetNodeData();
    configureCheckpointStrategy();
    checkpointStrategy->rollback(requester, cpId);
}

void BatteryNodeApp::initiateRollbackToLastCheckpoint(){
    NS_LOG_FUNCTION(this);

    configureCheckpointStrategy();
    checkpointStrategy->rollbackToLastCheckpoint();
}

void BatteryNodeApp::afterRollback(){
    NS_LOG_FUNCTION(this);

    try {

        //Reiniciando aplicação...
        StartApplication();

        NS_LOG_INFO("\nDepois do rollback...");
        printNodeData();

    } catch (NodeAsleepException& e) {
        //Operações interrompidas... Nó irá entrar em modo sleep. Nada mais a fazer.
    } catch (NodeDepletedException& e) {
        //Operações interrompidas... Nó irá entrar em modo depleted. Nada mais a fazer.
    } 
}

void BatteryNodeApp::generateEnergy(){
    NS_LOG_FUNCTION(this);
    
    double v = energyGenerator->getValue();
    battery.rechargeEnergy(v);

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", energia gerada por " 
                << getNodeName() << ": " << v << ". Energia restante: " 
                << to_string(battery.getRemainingEnergy()) << ".");

    checkModeChange();

    Simulator::Schedule(energyUpdateInterval,
                                &BatteryNodeApp::generateEnergy,
                                this);

}

void BatteryNodeApp::checkModeChange(){
    NS_LOG_FUNCTION(this);
    
    /*if (battery.getBatteryPercentage() == 0 && currentMode != Mode::DEPLETED){
        currentMode = Mode::DEPLETED;

        resetNodeData();

        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << getNodeName() << " entrou em modo DEPLETED.");
        throw NodeDepletedException("Bateria entrou em modo DEPLETED.");

    } else */
    
    if (battery.getBatteryPercentage() <= sleepModePercentage && currentMode == Mode::NORMAL){
        currentMode = Mode::SLEEP;

        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << getNodeName() << " entrou em modo SLEEP.");
        resetNodeData();

        throw NodeAsleepException("Nó entrou em modo SLEEP.");

    } else if (battery.getBatteryPercentage() > normalModePercentage && currentMode != Mode::NORMAL){
        currentMode = Mode::NORMAL;
        NS_LOG_INFO("\nAos " << Simulator::Now().As(Time::S) << ", " << getNodeName() << " entrou em modo NORMAL.");

        initiateRollbackToLastCheckpoint();
    }
}

void BatteryNodeApp::decreaseEnergy(double amount) {
    NS_LOG_FUNCTION(this);
    
    NS_ASSERT_MSG(
            (currentMode == Mode::SLEEP && amount == sleepEnergyConsumption) ||
            (currentMode == Mode::NORMAL), 
                    "Operação inválida! Bateria está em modo sleep e não pode realizar operações.");
    
    battery.decrementEnergy(amount);

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", energia consumida por " << getNodeName() << 
                ": " << amount << ". Energia restante: " << to_string(battery.getRemainingEnergy()) << ".");

    checkModeChange();
}

void BatteryNodeApp::decreaseCheckpointEnergy(){
    NS_LOG_FUNCTION(this);
    decreaseEnergy(createCheckpointConsumption);
}

void BatteryNodeApp::decreaseRollbackEnergy(){
    NS_LOG_FUNCTION(this);
    decreaseEnergy(rollbackEnergyConsumption);
}

void BatteryNodeApp::decreaseReadEnergy(){
    NS_LOG_FUNCTION(this);
    decreaseEnergy(receivePacketConsumption);
}

void BatteryNodeApp::decreaseSendEnergy(){
    NS_LOG_FUNCTION(this);
    decreaseEnergy(sendPacketConsumption);
}

void BatteryNodeApp::decreaseConnectEnergy(){
    NS_LOG_FUNCTION(this);
    decreaseEnergy(connectConsumption);
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
}

void BatteryNodeApp::decreaseSleepEnergy(){
    NS_LOG_FUNCTION(this);
    
    try {
        decreaseEnergy(sleepEnergyConsumption);
    } catch (NodeDepletedException& e) {
        //Nada a fazer
    }
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
    
}

void BatteryNodeApp::resetNodeData() {
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO("\nAntes do rollback...");
    printNodeData();

    StopApplication();
    
    udpHelper = nullptr;
    m_port = 0;
    m_seq = 0;
    checkpointStrategy = nullptr;
}

void BatteryNodeApp::printNodeData(){
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO("Dados de " << getNodeName() << ":" );
    NS_LOG_INFO(
        "battery.getBatteryPercentage() = " << battery.getBatteryPercentage()
        << ", currentMode = " << currentMode
        << ", idleEnergyConsumption = " << idleEnergyConsumption
        << ", sleepEnergyConsumption = " << sleepEnergyConsumption
        << ", energyUpdateInterval = " << energyUpdateInterval.As(Time::S)
        << ", energyGenerator->GetTypeId() = " << energyGenerator->GetTypeId()
        << ", rollbackInProgress = " << checkpointStrategy->isRollbackInProgress()
        << ", checkpointInProgress = " << checkpointStrategy->isCheckpointInProgress()
        << ", m_port = " << m_port
        << ", m_seq = " << m_seq
        << ", dependentAddresses.size() = " << checkpointStrategy->getDependentAddresses().size()
        << ", configFilename = " << configFilename);
    
    udpHelper->printData();
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

bool BatteryNodeApp::mayCheckpoint(){
    NS_LOG_FUNCTION(this);
    
    return !isDepleted() && !isSleeping() && !checkpointStrategy->isRollbackInProgress();
}

bool BatteryNodeApp::mayRemoveCheckpoint() {
    NS_LOG_FUNCTION(this);

    return !isDepleted() && !isSleeping(); 
}

json BatteryNodeApp::to_json() const {
    NS_LOG_FUNCTION(this);

    json j = CheckpointApp::to_json();
    j["m_port"] = m_port;
    j["m_seq"] = m_seq;

    return j;
}

void BatteryNodeApp::from_json(const json& j) {
    NS_LOG_FUNCTION(this);
    
    CheckpointApp::from_json(j);

    j.at("m_port").get_to(m_port);
    j.at("m_seq").get_to(m_seq); 
}

} // Namespace ns3
