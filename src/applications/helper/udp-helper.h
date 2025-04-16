/*
 * Copyright (c) 2008 INRIA
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
 * Author: Geoge Riley <riley@ece.gatech.edu>
 * Adapted from OnOffHelper by:
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef UDP_HELPER_H
#define UDP_HELPER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/udp-socket.h"
#include "ns3/socket.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/packet.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/message-data.h"
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;
using namespace std;

namespace ns3
{

class MessageData;

/** Payload referente a uma requisição para que outros nós iniciem procedimentos de rollback. */
const static string REQUEST_TO_START_ROLLBACK_COMMAND = "REQUEST_TO_START_ROLLBACK_COMMAND";

/** Payload referente a uma requisição de valor de um nó para outro. */
const static string REQUEST_VALUE = "REQUEST_VALUE";

/** Payload referente a uma resposta a uma requisição de valor feita por um nó. */
const static string RESPONSE_VALUE = "RESPONSE_VALUE";

/** Payload referente a um aviso de rollback concluído. */
const static string ROLLBACK_FINISHED_COMMAND = "ROLLBACK_FINISHED_COMMAND";

/** Payload referente a um aviso de checkpoint concluído. */
const static string CHECKPOINT_FINISHED_COMMAND = "CHECKPOINT_FINISHED_COMMAND";

/**
 * \ingroup helper
 * \brief Classe que auxilia no processo de gerenciamento de sockets e pacotes UDP.
 */
class UDPHelper : public Object
{

private:

    void HandleRead(Ptr<Socket> socket);

    /////////////////////////////////////////////////////////////////////////////
    //////          ATRIBUTOS    NÃO    ARMAZENADOS EM CHECKPOINTS         //////
    /////////////////////////////////////////////////////////////////////////////

    /* 
      Por questão de organização, aqui devem ser declarados os atributos que não devem ser armazenados em 
      checkpoints. Exemplos desses tipos de atributos incluem atributos físicos, como, a carga atual da bateria, 
      ou atributos fixos (que nunca mudam) de uma aplicação.
    */

    /// Traced Callback: transmitted packets.
    TracedCallback<Ptr<const Packet>> m_txTrace;

    /// Callbacks for tracing the packet Rx events
    TracedCallback<Ptr<const Packet>> m_rxTrace;

    /// Callbacks for tracing the packet Tx events, includes source and destination addresses
    TracedCallback<Ptr<const Packet>, const Address&, const Address&> m_txTraceWithAddresses;

    /// Callbacks for tracing the packet Rx events, includes source and destination addresses
    TracedCallback<Ptr<const Packet>, const Address&, const Address&> m_rxTraceWithAddresses;

    //!< Socket
    Ptr<Socket> m_socket; 

    //Callback a ser chamado ao receber uma mensagem referente às regras normais de negócio.
    Callback<void, Ptr<MessageData>> receiveCallback;

    //Callback de interceptação do protocolo de checkpointing a ser chamado ao receber uma mensagem.
    Callback<bool, Ptr<MessageData>> protocolReceiveCallback;

    //Callback de interceptação do protocolo de checkpointing a ser chamado ao enviar uma mensagem.
    Callback<bool, Ptr<MessageData>> protocolSendCallback;

    //////////////////////////////////////////////////////////////
    //////       ATRIBUTOS ARMAZENADOS EM CHECKPOINTS       //////
    //////////////////////////////////////////////////////////////

    //Somente atributos de aplicação serão armazenados em checkpoints

    Address m_address;     //!< The node's address
    uint16_t m_port;       //!< The node's port
    Address m_local;       //!< local multicast address
    uint64_t m_totalTx;    //!< Total bytes sent
    uint32_t m_sent;       //!< Counter for sent messages
    uint32_t m_received;   //!< Contador de mensagens recebidas (não necessariamente processadas)
    string m_nodeName;     //nome do nó que está usando este helper

public:

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    UDPHelper();

    ~UDPHelper() override;

    /** 
     * Configura um socket e atribui um endereço IP ao nó.
     * 
     * @node Nó que está se conectando.
     * @nodeName Nome do nó que está se conectando.
    */
    void configureClient(Ptr<Node> node, string nodeName);

    /** 
     * Atribui um endereço IP ao servidor e fica aguardando mensagens de outros nós em uma porta específica.
     * 
     * @node Nó que está se conectando.
     * @nodeName Nome do nó que está se conectando.
     * @port Porta através da qual as mensagens serão recebidas.
    */
    void configureServer(Ptr<Node> node, string nodeName, uint16_t port);

    /** Encerra a conexão via socket.*/
    void terminateConnection();

    /** 
     * Envia um pacote para um nó.
     * 
     * @param command comando que indica o tipo de mensagem.
     * @param d dado que será transmitido na mensagem. 0 caso não seja necessário.
     * @param to Indica para qual endereço o pacote será enviado.
     * */
    Ptr<MessageData> send(string command, int d, Address to);

    /** 
     * Envia um pacote para um nó.
     * 
     * @param command comando que indica o tipo de mensagem.
     * @param d dado que será transmitido na mensagem. 0 caso não seja necessário.
     * @param ip IP de destino.
     * @param port Porta de destino.
     * */
    Ptr<MessageData> send(string command, int d, Ipv4Address ip, uint16_t port);

    /** Imprime os dados da classe para fins de debug. */
    void printData();

    //Especifica como deve ser feita a conversão desta classe em JSON
    friend void to_json(json& j, const UDPHelper& obj);

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    friend void from_json(const json& j, UDPHelper& obj);

    /**************  GETTERS  ***************/

    Address getAddress() const;
    uint16_t getPort() const;
    uint64_t getTotalBytesSent() const;
    uint32_t getSentMessagesCounter() const;
    uint32_t getReceivedMessagesCounter() const;
    string getNodeName() const;
    bool isDisconnected() const;

    /**************  SETTERS  ***************/

    /** 
     * Atribui um método de callback, a ser chamado quando um pacote for recebido 
     * no socket que está sendo usado. 
     * */
    void setReceiveCallback(Callback<void, Ptr<MessageData>> callback);

    /** 
     * Atribui um método de callback, a ser chamado quando um pacote for recebido 
     * no socket que está sendo usado. Esse callback é referente ao protocolo que
     * estiver sendo testado na simulação. A mensagem será processada primeiro pelo
     * protocolo, e só depois pela aplicação.
     * */
    void setProtocolReceiveCallback(Callback<bool, Ptr<MessageData>> callback);

    /** 
     * Atribui um método de callback, a ser chamado quando um pacote for enviado 
     * no socket que está sendo usado. Esse callback é referente ao protocolo que
     * estiver sendo testado na simulação. Toda mensagem enviada será interceptada
     * pelo protocolo para análise.
     * */
    void setProtocolSendCallback(Callback<bool, Ptr<MessageData>> callback);

    void setAddress(const ns3::Address& address);
    void setPort(uint16_t port);
    void setNodeName(const std::string& name);

};

} // namespace ns3

#endif /* UDP_HELPER_H */
