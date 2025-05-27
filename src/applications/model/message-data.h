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

#ifndef MESSAGE_DATA_H
#define MESSAGE_DATA_H

#include <ns3/nstime.h>
#include "ns3/object.h"
#include "ns3/type-id.h"
#include <ns3/assert.h>
#include <ns3/double.h>
#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/trace-source-accessor.h>
#include "ns3/address.h"
#include <string>
#include "ns3/core-module.h"
#include "ns3/ipv4-address.h"
#include "ns3/inet-socket-address.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

namespace ns3
{

class MessageData : public Object
{

  private:

    Address from; //sender of the message
    Address to;   //message destination

    uint64_t uid;            //packet's uid
    uint32_t sequenceNumber; //sequence number of the message
    string command;          //indica o tipo de mensagem ou comando a ser executado pelo receptor
    int data;                //dado que complementa o comando da mensagem
    uint32_t size;           //tamanho em bytes da mensagem
    string piggyBackedInfo;  //Informações de controle que podem ser enviadas junto de mensagens de aplicação


  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    // Construtores
    MessageData();
    MessageData(uint64_t uid, const ns3::Address& from, const ns3::Address& to, uint32_t sequenceNumber, 
      const std::string& command, int data, uint32_t size);
    ~MessageData() override;

    //Especifica como deve ser feita a conversão desta classe em JSON
    friend void to_json(json& j, const MessageData& obj);

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    friend void from_json(const json& j, MessageData& obj);

    // Getters
    Address GetFrom() const { return from; }
    Ipv4Address GetFromIpv4() const { return InetSocketAddress::ConvertFrom(from).GetIpv4(); }
    Address GetTo() const { return to; }
    uint32_t GetSequenceNumber() const { return sequenceNumber; }
    std::string GetCommand() const { return command; }
    std::string GetCompleteCommand() const { return command + (data > 0 ? " " + to_string(data) : ""); }
    std::string GetPiggyBackedInfo() const { return piggyBackedInfo; }
    int GetData() const { return data; }
    uint32_t GetSize() const { return size; }
    uint64_t GetUid() const { return uid; }

    // Setters
    void SetFrom(const Address& addr) { from = addr; }
    void SetTo(const Address& addr) { to = addr; }
    void SetSequenceNumber(uint32_t seqNum) { sequenceNumber = seqNum; }
    void SetCommand(const std::string& cmd) { command = cmd; }
    void SetData(int value) { data = value; }
    void SetPiggyBackedInfo(std::string info) { piggyBackedInfo = info; }
    void SetSize(uint32_t msgSize) { size = msgSize; }
    void SetUid(uint64_t msgUid) { uid = msgUid; }

};

} // namespace ns3

#endif /* MESSAGE_DATA_H */
