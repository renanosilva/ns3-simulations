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

#include "checkpoint-app.h"
#include "ns3/inet-socket-address.h"
#include "ns3/global-sync-clocks-strategy.h"
#include "ns3/decentralized-recovery-protocol.h"
#include "ns3/efficient-assync-recovery-protocol.h"
#include "ns3/log-utils.h"
#include "ns3/node-depleted-exception.h"
#include "ns3/node-asleep-exception.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CheckpointApp");
NS_OBJECT_ENSURE_REGISTERED(CheckpointApp);

TypeId
CheckpointApp::GetTypeId()
{
    NS_LOG_FUNCTION("CheckpointApp::GetTypeId()");

    static TypeId tid =
        TypeId("ns3::CheckpointApp")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<CheckpointApp>();
    
    return tid;
}

CheckpointApp::CheckpointApp()
{
    NS_LOG_FUNCTION(this);
}

CheckpointApp::~CheckpointApp()
{
    NS_LOG_FUNCTION(this);
}

void CheckpointApp::loadConfigurations() {
    NS_LOG_FUNCTION(this);

    string propertyPrefix = "nodes." + getNodeName() + ".";

    bool hasBattery = configHelper->GetBoolProperty(propertyPrefix + "has-battery", false);
    
    if (hasBattery){
        if (battery == nullptr)
            battery = Create<Battery>();
        
        energyUpdateInterval = Seconds(configHelper->GetDoubleProperty(propertyPrefix + "energy-update-interval", -1));
        sleepEnergyConsumption = configHelper->GetDoubleProperty(propertyPrefix + "sleep-energy", -1);
        idleEnergyConsumption = configHelper->GetDoubleProperty(propertyPrefix + "idle-energy", -1);
        rollbackEnergyConsumption = configHelper->GetDoubleProperty(propertyPrefix + "rollback-energy", -1);
        createCheckpointConsumption = configHelper->GetDoubleProperty(propertyPrefix + "create-checkpoint-energy", -1);
        receivePacketConsumption = configHelper->GetDoubleProperty(propertyPrefix + "receive-packet-energy", -1);
        sendPacketConsumption = configHelper->GetDoubleProperty(propertyPrefix + "send-packet-energy", -1);
        connectConsumption = configHelper->GetDoubleProperty(propertyPrefix + "connect-energy", -1);
        sleepModePercentage = configHelper->GetDoubleProperty(propertyPrefix + "sleep-mode-percentage", 10.0);
        normalModePercentage = configHelper->GetDoubleProperty(propertyPrefix + "normal-mode-percentage", 20.0);
    
        if (!energyGenerator)
            configureEnergyGenerator();
    }

    if (!checkpointStrategy)
        configureCheckpointStrategy();
}

void CheckpointApp::configureEnergyGenerator() {
    NS_LOG_FUNCTION(this);

    string property = "nodes." + getNodeName() + ".energy-generator";
    string energyGeneratorName = configHelper->GetStringProperty(property);

    if (energyGeneratorName == "FixedEnergyGenerator"){
        
        string valueProperty = "nodes." + getNodeName() + ".energy-fixed-value";
        double value = configHelper->GetDoubleProperty(valueProperty);

        energyGenerator = Create<FixedEnergyGenerator>(value);    

    } else if (energyGeneratorName == "CircularEnergyGenerator") {
        
        energyGenerator = Create<CircularEnergyGenerator>();

    } /*else {
        NS_ABORT_MSG("Não foi possível identificar a estratégia de geração de energia de " << getNodeName());
    }*/
    
    if (energyGenerator != nullptr){
        //Agendando geração de energia
        Simulator::Schedule(energyUpdateInterval,
            &CheckpointApp::generateEnergy,
            this);

        //Agendando desconto contínuo de energia referente ao funcionamento básico do dispositivo
        Simulator::Schedule(energyUpdateInterval,
                &CheckpointApp::decreaseCurrentModeEnergy,
                this);
    }
    
}

void CheckpointApp::configureCheckpointStrategy() {
    NS_LOG_FUNCTION(this);
    
    string property = "simulation.checkpoint-strategy";
    string checkpointStrategyName = configHelper->GetStringProperty(property);

    if (checkpointStrategyName == "GlobalSyncClocksStrategy"){
        
        string intervalProperty = "simulation.checkpoint-interval";
        double checkpointInterval = configHelper->GetDoubleProperty(intervalProperty);

        string timeoutProperty = "simulation.checkpoint-timeout";
        double checkpointTimeout = configHelper->GetDoubleProperty(timeoutProperty);

        checkpointStrategy = Create<GlobalSyncClocksStrategy>(Seconds(checkpointInterval), Seconds(checkpointTimeout), this);
        checkpointStrategy->startCheckpointing();
    
    } else if (checkpointStrategyName == "DecentralizedRecoveryProtocol"){

        string intervalProperty = "nodes." + getNodeName() + ".checkpoint-interval";
        double checkpointInterval = configHelper->GetDoubleProperty(intervalProperty);

        checkpointStrategy = Create<DecentralizedRecoveryProtocol>(Seconds(checkpointInterval), this);
        checkpointStrategy->startCheckpointing();

    } else if (checkpointStrategyName == "EfficientAssyncRecoveryProtocol") {

        string intervalProperty = "nodes." + getNodeName() + ".checkpoint-interval";
        double checkpointInterval = configHelper->GetDoubleProperty(intervalProperty);

        checkpointStrategy = Create<EfficientAssyncRecoveryProtocol>(Seconds(checkpointInterval), totalNodesQuantity, this);
        checkpointStrategy->startCheckpointing();

    } else {
        NS_ABORT_MSG("Não foi possível identificar a estratégia de checkpoint de " << getNodeName());
    }
}

void CheckpointApp::initiateRollback(Address requester, int cpId, string piggyBackedInfo){
    NS_LOG_FUNCTION(this);

    resetNodeData();
    configureCheckpointStrategy();
    checkpointStrategy->rollback(requester, cpId, piggyBackedInfo);
}

void CheckpointApp::initiateRollbackToLastCheckpoint(){
    NS_LOG_FUNCTION(this);
    
    configureCheckpointStrategy();
    checkpointStrategy->rollbackToLastCheckpoint();
}

void CheckpointApp::resetNodeData() {

}

void CheckpointApp::printNodeData(){

}

Ptr<MessageData> CheckpointApp::send(string command, int d, Address to, bool replay){
    Ptr<MessageData> md = udpHelper->send(command, d, to, replay);

    if (md != nullptr && !replay){
        utils::logRegularMessageSent(getNodeName(), md);
        decreaseSendEnergy();
    }

    return md;
}

Ptr<MessageData> CheckpointApp::send(string command, int d, Ipv4Address ip, uint16_t port, bool replay){
    Ptr<MessageData> md = udpHelper->send(command, d, ip, port, replay);

    if (md != nullptr && !replay){
        utils::logRegularMessageSent(getNodeName(), md);
        decreaseSendEnergy();
    }
    
    return md;
}

void CheckpointApp::replayReceive(MessageData md){
    udpHelper->replayReceive(md);
}

void
CheckpointApp::StartApplication()
{
    NS_LOG_FUNCTION(this);
}

void
CheckpointApp::StopApplication()
{
    NS_LOG_FUNCTION(this);
}

void CheckpointApp::beforeCheckpointDiscard(){
    
}

void CheckpointApp::afterCheckpointDiscard(){
    
}

void CheckpointApp::beforeRollback(){
    
}

void CheckpointApp::afterRollback(){
    NS_LOG_FUNCTION(this);

    try {

        //Reiniciando aplicação...
        StartApplication();

    } catch (NodeAsleepException& e) {
        //Operações interrompidas... Nó irá entrar em modo sleep. Nada mais a fazer.
    } catch (NodeDepletedException& e) {
        //Operações interrompidas... Nó irá entrar em modo depleted. Nada mais a fazer.
    } 
}

void CheckpointApp::generateEnergy(){
    NS_LOG_FUNCTION(this);
    
    if (energyGenerator != nullptr && battery != nullptr){
        double v = energyGenerator->getValue();
        battery->rechargeEnergy(v);

        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", energia gerada por " 
                    << getNodeName() << ": " << v << ". Energia restante: " 
                    << to_string(battery->getRemainingEnergy()) << ".");

        checkModeChange();

        Simulator::Schedule(energyUpdateInterval,
                                    &CheckpointApp::generateEnergy,
                                    this);
    }
}

void CheckpointApp::checkModeChange(){
    NS_LOG_FUNCTION(this);
    
    if (battery != nullptr){
        /*if (battery->getBatteryPercentage() == 0 && currentMode != Mode::DEPLETED){
            currentMode = Mode::DEPLETED;

            resetNodeData();

            NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << getNodeName() << " entrou em modo DEPLETED.");
            throw NodeDepletedException("Bateria entrou em modo DEPLETED.");

        } else */
        
        if (battery->getBatteryPercentage() <= sleepModePercentage && currentMode == EnergyMode::NORMAL){
            currentMode = EnergyMode::SLEEP;

            NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << getNodeName() << " entrou em modo SLEEP.");
            resetNodeData();

            throw NodeAsleepException("Nó entrou em modo SLEEP.");

        } else if (battery->getBatteryPercentage() > normalModePercentage && currentMode != EnergyMode::NORMAL){
            currentMode = EnergyMode::NORMAL;
            NS_LOG_INFO("\nAos " << Simulator::Now().As(Time::S) << ", " << getNodeName() << " entrou em modo NORMAL.");

            initiateRollbackToLastCheckpoint();
        }
    }
}

void CheckpointApp::decreaseEnergy(double amount) {
    NS_LOG_FUNCTION(this);
    
    if (battery != nullptr){
        NS_ASSERT_MSG(
            (currentMode == EnergyMode::SLEEP && amount == sleepEnergyConsumption) ||
            (currentMode == EnergyMode::NORMAL), 
                    "Operação inválida! Bateria está em modo sleep e não pode realizar operações.");
    
        battery->decrementEnergy(amount);

        NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", energia consumida por " << getNodeName() << 
                    ": " << amount << ". Energia restante: " << to_string(battery->getRemainingEnergy()) << ".");

        checkModeChange();
    }
}

void CheckpointApp::decreaseCheckpointEnergy(){
    NS_LOG_FUNCTION(this);
    decreaseEnergy(createCheckpointConsumption);
}

void CheckpointApp::decreaseRollbackEnergy(){
    NS_LOG_FUNCTION(this);
    decreaseEnergy(rollbackEnergyConsumption);
}

void CheckpointApp::decreaseReadEnergy(){
    NS_LOG_FUNCTION(this);
    decreaseEnergy(receivePacketConsumption);
}

void CheckpointApp::decreaseSendEnergy(){
    NS_LOG_FUNCTION(this);
    decreaseEnergy(sendPacketConsumption);
}

void CheckpointApp::decreaseConnectEnergy(){
    NS_LOG_FUNCTION(this);
    decreaseEnergy(connectConsumption);
}

void CheckpointApp::decreaseIdleEnergy(){
    NS_LOG_FUNCTION(this);

    try {
        decreaseEnergy(idleEnergyConsumption);
    } catch (NodeAsleepException& e) {
        //Nada a fazer
    } catch (NodeDepletedException& e) {
        //Nada a fazer
    } 
}

void CheckpointApp::decreaseSleepEnergy(){
    NS_LOG_FUNCTION(this);
    
    try {
        decreaseEnergy(sleepEnergyConsumption);
    } catch (NodeDepletedException& e) {
        //Nada a fazer
    }
}

void CheckpointApp::decreaseCurrentModeEnergy(){
    NS_LOG_FUNCTION(this);
    
    if (currentMode == EnergyMode::NORMAL){
        decreaseIdleEnergy();
    } else if (currentMode == EnergyMode::SLEEP){
        decreaseSleepEnergy();
    }

    Simulator::Schedule(energyUpdateInterval,
                                &CheckpointApp::decreaseCurrentModeEnergy,
                                this);
    
}

EnergyMode CheckpointApp::getCurrentMode(){
    NS_LOG_FUNCTION(this);
    return currentMode;
}

bool CheckpointApp::isSleeping(){
    NS_LOG_FUNCTION(this);
    return currentMode == SLEEP;
}

bool CheckpointApp::isDepleted(){
    NS_LOG_FUNCTION(this);
    return currentMode == DEPLETED;
}

Time CheckpointApp::getEnergyUpdateInterval(){
    NS_LOG_FUNCTION(this);
    return energyUpdateInterval;
}

bool CheckpointApp::mayCheckpoint(){
    NS_LOG_FUNCTION(this);
    
    return !isDepleted() && !isSleeping() && !checkpointStrategy->isRollbackInProgress();
}

bool CheckpointApp::mayRemoveCheckpoint() {
    NS_LOG_FUNCTION(this);

    return !isDepleted() && !isSleeping(); 
}

bool CheckpointApp::hasEnoughEnergy(double requiredEnergy){
    NS_LOG_FUNCTION(this);

    if (battery == nullptr)
        return true;

    //Energia na qual o nó entra em modo sleep
    double sleepEnergy = (sleepModePercentage/100)*battery->getMaxCapacity();

    return battery->getRemainingEnergy() > (sleepEnergy + requiredEnergy);
}

bool CheckpointApp::hasEnoughEnergyToCheckpoint(){
    NS_LOG_FUNCTION(this);
    return hasEnoughEnergy(createCheckpointConsumption);
}

bool CheckpointApp::hasEnoughEnergyToReceivePacket(){
    NS_LOG_FUNCTION(this);
    return hasEnoughEnergy(receivePacketConsumption);
}

bool CheckpointApp::hasEnoughEnergyToSendPacket(){
    NS_LOG_FUNCTION(this);
    return hasEnoughEnergy(sendPacketConsumption);
}

ApplicationType CheckpointApp::getApplicationType(){
    return applicationType;
}

json CheckpointApp::to_json() const {
    NS_LOG_FUNCTION(this);
    
    json j = Application::to_json();

    j["udpHelper"] = *udpHelper;
    j["nodeName"] = nodeName;
    j["configFilename"] = configFilename;
    j["checkpointStrategy"] = checkpointStrategy->to_json();

    return j;
}

void CheckpointApp::from_json(const json& j) {
    NS_LOG_FUNCTION(this);

    Application::from_json(j);

    udpHelper = Create<UDPHelper>();

    j.at("udpHelper").get_to(*udpHelper); 
    j.at("nodeName").get_to(nodeName);
    j.at("configFilename").get_to(configFilename);

    checkpointStrategy->from_json(j.at("checkpointStrategy"));
}

string CheckpointApp::getNodeName(){
    NS_LOG_FUNCTION(this);
    return nodeName;
}

} // Namespace ns3
