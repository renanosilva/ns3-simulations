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

#include "client-node-app.h"

#include "seq-ts-header.h"

#include "ns3/sync-predefined-times-checkpoint.h"

#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/ipv4-address.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/uinteger.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-address.h"
#include "ns3/core-module.h"
#include "ns3/address.h"
#include "ns3/log-utils.h"
#include "ns3/json-utils.h"
#include "ns3/utils.h"

#include <cstdio>
#include <cstdlib>

using namespace std;

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ClientApp");
NS_OBJECT_ENSURE_REGISTERED(ClientNodeApp);

TypeId
ClientNodeApp::GetTypeId()
{
    NS_LOG_FUNCTION("ClientNodeApp::GetTypeId()");

    static TypeId tid =
        TypeId("ns3::ClientNodeApp")
            .SetParent<CheckpointApp>()
            .SetGroupName("Applications")
            .AddConstructor<ClientNodeApp>()
            .AddAttribute(
                "MaxPackets",
                "The maximum number of packets the application will send (zero means infinite)",
                UintegerValue(100),
                MakeUintegerAccessor(&ClientNodeApp::m_count),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute("Interval",
                            "The time to wait between packets",
                            TimeValue(Seconds(1.0)),
                            MakeTimeAccessor(&ClientNodeApp::m_interval),
                            MakeTimeChecker())
            .AddAttribute("RemoteAddresses",
                            "List of destination addresses in 'IP;IP;IP...' format",
                            StringValue(""), // Valor padrão: vazio
                            MakeStringAccessor(&ClientNodeApp::GetPeerAddresses, &ClientNodeApp::SetPeerAddresses),
                            MakeStringChecker()) // Verifica se é uma string válida
            .AddAttribute("RemotePort",
                            "The destination port of the outbound packets",
                            UintegerValue(0),
                            MakeUintegerAccessor(&ClientNodeApp::m_peerPort),
                            MakeUintegerChecker<uint16_t>())
            .AddAttribute("NodeName",
                            "This node's name. ",
                            StringValue(""),
                            MakeStringAccessor(&ClientNodeApp::nodeName),
                            MakeStringChecker())
            .AddAttribute("ConfigFilename",
                            "This node's config filename. ",
                            StringValue(""),
                            MakeStringAccessor(&ClientNodeApp::configFilename),
                            MakeStringChecker());

    NS_LOG_FUNCTION("Fim do método");

    return tid;
}

ClientNodeApp::ClientNodeApp()
{
    NS_LOG_FUNCTION(this);
    
    udpHelper = Create<UDPHelper>();
    m_sendEvent = EventId();

    NS_LOG_FUNCTION("Fim do método");
}

ClientNodeApp::~ClientNodeApp()
{
    NS_LOG_FUNCTION(this);
    
    udpHelper = nullptr;
    Simulator::Cancel(m_sendEvent);

    NS_LOG_FUNCTION("Fim do método");
}

void
ClientNodeApp::SetRemote(Address ip, uint16_t port)
{
    NS_LOG_FUNCTION(this << ip << port);
    
    udpHelper->SetAttribute("RemoteAddress", AddressValue(ip));
    udpHelper->SetAttribute("RemotePort", UintegerValue(port));

    NS_LOG_FUNCTION("Fim do método");
}

void
ClientNodeApp::SetRemote(Address addr)
{
    NS_LOG_FUNCTION(this << addr);
    
    udpHelper->SetAttribute("RemoteAddress", AddressValue(addr));

    NS_LOG_FUNCTION("Fim do método");
}

void
ClientNodeApp::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (!configHelper){
        configHelper = Create<ConfigHelper>(configFilename);
    }

    loadConfigurations();

    if (udpHelper->isDisconnected()){
        udpHelper->configureClient(GetNode(), getNodeName());
    }

    udpHelper->setReceiveCallback(MakeCallback(&ClientNodeApp::HandleRead, this));

    //Agendando envio de mensagens
    m_sendEvent = Simulator::Schedule(Seconds(0.0), &ClientNodeApp::Send, this);

    NS_LOG_INFO(getNodeName() << " conectado.");
    NS_LOG_FUNCTION("Fim do método");
}

void
ClientNodeApp::StopApplication()
{
    NS_LOG_FUNCTION(this);
    
    Simulator::Cancel(m_sendEvent);
    udpHelper->terminateConnection();

    NS_LOG_FUNCTION("Fim do método");
}

void
ClientNodeApp::Send()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_sendEvent.IsExpired());

    for (Ipv4Address ip : m_peerAddresses){
        
        Ptr<MessageData> md = udpHelper->send(REQUEST_VALUE, 0, ip, m_peerPort);
        utils::logRegularMessageSent(getNodeName(), md);

    }

    if (udpHelper->getSentMessagesCounter() < m_count || m_count == 0){
        //Agendando próximo envio
        m_sendEvent = Simulator::Schedule(m_interval, &ClientNodeApp::Send, this);
    }

    NS_LOG_FUNCTION("Fim do método");
}

void ClientNodeApp::HandleRead(Ptr<MessageData> md)
{
    NS_LOG_FUNCTION(this << md);

    utils::logMessageReceived(getNodeName(), md);

    if (md->GetCommand() == RESPONSE_VALUE){
        last_seq = md->GetData();
    }

    if (md->GetCommand() == REQUEST_TO_START_ROLLBACK_COMMAND){
        rollbackStarterIp = InetSocketAddress::ConvertFrom(md->GetFrom()).GetIpv4();
        resetNodeData();
        configureCheckpointStrategy();
        checkpointStrategy->startRollback(md->GetData());
    }

    NS_LOG_FUNCTION("Fim do método");
}

void ClientNodeApp::loadConfigurations() {
    if (!checkpointStrategy){
        configureCheckpointStrategy();
    }
}

void ClientNodeApp::configureCheckpointStrategy() {
    NS_LOG_FUNCTION(this);
    
    string property = "nodes.client-nodes." + getNodeName() + ".checkpoint-strategy";
    string checkpointStrategyName = configHelper->GetStringProperty(property);

    if (checkpointStrategyName == "SyncPredefinedTimesCheckpoint"){
        
        string intervalProperty = "nodes.client-nodes." + getNodeName() + ".checkpoint-interval";
        double checkpointInterval = configHelper->GetDoubleProperty(intervalProperty);

        checkpointStrategy = Create<SyncPredefinedTimesCheckpoint>(Seconds(checkpointInterval), this);
        checkpointStrategy->startCheckpointing();
    
    } else {
        NS_ABORT_MSG("Não foi possível identificar a estratégia de checkpoint de " << getNodeName());
    }

    NS_LOG_FUNCTION("Fim do método");
}

void ClientNodeApp::notifyNodesAboutRollbackConcluded(){
    NS_LOG_FUNCTION(this);
    
    Ptr<MessageData> md = udpHelper->send(ROLLBACK_FINISHED_COMMAND, 0, rollbackStarterIp, m_peerPort);
    utils::logRegularMessageSent(getNodeName(), md);

    rollbackStarterIp = Ipv4Address::GetAny();

    NS_LOG_FUNCTION("Fim do método");
}

void ClientNodeApp::resetNodeData() {
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO("\nAntes do rollback...");
    printNodeData();
    
    StopApplication();
    udpHelper = nullptr;
    checkpointStrategy = nullptr;

    m_sendEvent = EventId();
    m_count = 0;
    m_interval = Time(); 
    last_seq = 0;
    m_peerAddresses = vector<Ipv4Address>();
    m_peerPort = 0;

    NS_LOG_FUNCTION("Fim do método");
}

bool ClientNodeApp::mayCheckpoint(){
    NS_LOG_FUNCTION(this);
    
    if (rollbackInProgress){
        return false;
    } else {
        return true;
    }
}

void ClientNodeApp::beforeRollback(){
    NS_LOG_FUNCTION(this);
    
    rollbackInProgress = true;
    
    NS_LOG_FUNCTION("Fim do método");
}

void ClientNodeApp::afterRollback(){
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << getNodeName() 
            << " concluiu o procedimento de rollback.");

    //Reiniciando aplicação...
    StartApplication();

    NS_LOG_INFO("\nApós o rollback...");
    printNodeData();

    notifyNodesAboutRollbackConcluded();
    rollbackInProgress = false;

    NS_LOG_FUNCTION("Fim do método");
}

uint64_t
ClientNodeApp::GetTotalTx() const
{
    NS_LOG_FUNCTION(this);
    return udpHelper->getTotalBytesSent();
}

json ClientNodeApp::to_json() const {
    NS_LOG_FUNCTION(this);

    json j = CheckpointApp::to_json();
    j["udpHelper"] = *udpHelper;
    j["m_peerAddresses"] = m_peerAddresses;
    j["m_peerPort"] = m_peerPort;
    j["m_count"] = m_count;
    j = utils::timeToJson(j, "m_interval", m_interval);
    j["last_seq"] = last_seq;
    
    NS_LOG_FUNCTION("Fim do método");

    return j;
}

void ClientNodeApp::from_json(const json& j) {
    NS_LOG_FUNCTION(this);
    
    CheckpointApp::from_json(j);

    udpHelper = Create<UDPHelper>();
    j.at("udpHelper").get_to(*udpHelper); 
    j.at("m_peerAddresses").get_to(m_peerAddresses); 
    j.at("m_peerPort").get_to(m_peerPort); 
    j.at("m_count").get_to(m_count); 
    m_interval = utils::jsonToTime(j, "m_interval"); 
    j.at("last_seq").get_to(last_seq);

    NS_LOG_FUNCTION("Fim do método");
}

void ClientNodeApp::printNodeData(){
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO("Dados de " << getNodeName() << ":" );
    NS_LOG_INFO(
        "m_count = " << m_count
        << ", m_interval = " << m_interval.As(Time::S)
        << ", m_peerAddresses = " << utils::convertAddressesToString(m_peerAddresses)
        << ", m_peerPort = " << m_peerPort
        << ", m_sendEvent = " << m_sendEvent.GetTs()
        << ", rollbackInProgress = " << rollbackInProgress
        << ", last_seq = " << last_seq
        << ", configFilename = " << configFilename);

    udpHelper->printData();

    NS_LOG_FUNCTION("Fim do método");
}

void ClientNodeApp::SetPeerAddresses(string addressList)
{
    m_peerAddresses.clear();
    istringstream iss(addressList);
    string token;

    while (std::getline(iss, token, ';')) // Lê cada endereço separado por ";"
    {
        m_peerAddresses.push_back(Ipv4Address(token.c_str()));
    }
}

string ClientNodeApp::GetPeerAddresses() const {
    ostringstream oss;
    
    for (size_t i = 0; i < m_peerAddresses.size(); i++){
        
        if (i > 0)
            oss << ";"; // Separador entre IPs

        oss << m_peerAddresses[i];

    }

    return oss.str();
}

} // Namespace ns3
