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

#include "message-data.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MessageData");
NS_OBJECT_ENSURE_REGISTERED(MessageData);

TypeId
MessageData::GetTypeId()
{
    NS_LOG_FUNCTION("MessageData::GetTypeId()");

    static TypeId tid =
        TypeId("ns3::MessageData")
            .AddConstructor<MessageData>()
            .SetParent<Object>()
            .SetGroupName("Message")
            .AddAttribute("From",
                "Sender of the message",
                AddressValue(),
                MakeAddressAccessor(&MessageData::from),
                MakeAddressChecker())
            .AddAttribute("To",
                "Message destination",
                AddressValue(),
                MakeAddressAccessor(&MessageData::to),
                MakeAddressChecker())
            .AddAttribute("Uid",
                          "Packet's UId",
                          UintegerValue(0), 
                          MakeUintegerAccessor(&MessageData::uid),
                          MakeUintegerChecker<uint64_t>())
            .AddAttribute("SequenceNumber",
                          "Sequence number of the message",
                          UintegerValue(0), 
                          MakeUintegerAccessor(&MessageData::sequenceNumber),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("Command",
                            "Indica o tipo de mensagem ou comando a ser executado pelo receptor",
                            StringValue(""), 
                            MakeStringAccessor(&MessageData::command),
                            MakeStringChecker())
            .AddAttribute("Data",
                          "Dado que complementa o comando da mensagem",
                          UintegerValue(0), 
                          MakeUintegerAccessor(&MessageData::data),
                          MakeUintegerChecker<int>())
            .AddAttribute("Size",
                          "Tamanho em bytes da mensagem",
                          UintegerValue(0), 
                          MakeUintegerAccessor(&MessageData::size),
                          MakeUintegerChecker<uint32_t>());

    NS_LOG_FUNCTION("Fim do método");
    return tid;
}

MessageData::MessageData(){
    NS_LOG_FUNCTION(this);

    from = Ipv4Address::GetAny();
    to = Ipv4Address::GetAny();
    sequenceNumber = 0;
    command = "";
    data = 0;
    size = 0;

    NS_LOG_FUNCTION("Fim do método");
}

MessageData::MessageData(uint64_t uid, const ns3::Address& from, const ns3::Address& to, 
    uint32_t sequenceNumber, const std::string& command, 
    int data, uint32_t size)
    : from(from), to(to), sequenceNumber(sequenceNumber), 
    command(command), data(data), size(size) {

}

MessageData::~MessageData()
{
    NS_LOG_FUNCTION(this);
}

void MessageData::addPiggyBackedInfo(std::string info){
    if (piggyBackedInfo.empty()){
        piggyBackedInfo = info;
    } else {
        piggyBackedInfo.append(" " + info);
    }
}

int MessageData::GetFirstPiggyBackedValue(){
    istringstream iss(piggyBackedInfo);
    string command;
    int value;
    iss >> command >> value;
    return value;
}

string MessageData::getPiggyBackedValue(string param){
    map<std::string, std::string> params;
    istringstream iss(piggyBackedInfo);
    string key, value;

    while (iss >> key >> value) {
        params[key] = value;
    }
    
    auto it = params.find(param);
    
    if (it != params.end()) {
        return it->second;
    }
    
    return "";
}

void to_json(json& j, const MessageData& obj) {
    NS_LOG_FUNCTION("MessageData::to_json");
    
    j = nlohmann::json{
        {"from", obj.from}, 
        {"to", obj.to},
        {"uid", obj.uid},
        {"sequenceNumber", obj.sequenceNumber},
        {"command", obj.command},
        {"data", obj.data},
        {"size", obj.size}
    };
}

void from_json(const json& j, MessageData& obj) {
    NS_LOG_FUNCTION("MessageData::from_json");
    
    j.at("from").get_to(obj.from);
    j.at("to").get_to(obj.to);
    j.at("uid").get_to(obj.uid);
    j.at("sequenceNumber").get_to(obj.sequenceNumber);
    j.at("command").get_to(obj.command);
    j.at("data").get_to(obj.data);
    j.at("size").get_to(obj.size);
}

void to_json(json& j, const Ptr<MessageData>& obj){
    if (obj == nullptr){
        j = nullptr;
        return;
    }
    
    j = nlohmann::json{
        {"from", obj->from}, 
        {"to", obj->to},
        {"uid", obj->uid},
        {"sequenceNumber", obj->sequenceNumber},
        {"command", obj->command},
        {"data", obj->data},
        {"size", obj->size},
        {"piggyBackedInfo", obj->piggyBackedInfo}
    };
}

void from_json(const json& j, Ptr<MessageData>& obj){
    if (j.is_null()){
        obj = nullptr;
        return;
    }
    
    obj = CreateObject<MessageData>();
    j.at("from").get_to(obj->from);
    j.at("to").get_to(obj->to);
    j.at("uid").get_to(obj->uid);
    j.at("sequenceNumber").get_to(obj->sequenceNumber);
    j.at("command").get_to(obj->command);
    j.at("data").get_to(obj->data);
    j.at("size").get_to(obj->size);
    j.at("piggyBackedInfo").get_to(obj->piggyBackedInfo);
}

} // Namespace ns3
