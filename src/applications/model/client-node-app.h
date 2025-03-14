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

#ifndef CLIENT_NODE_APP_H
#define CLIENT_NODE_APP_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ipv4-address.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/checkpoint-strategy.h"
#include "ns3/checkpoint-app.h"
#include "ns3/json-utils.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace ns3
{

class Socket;
class Packet;

/**
 * \ingroup udpclientserver
 * \brief An Udp client. Sends UDP packet carrying sequence number and time stamp
 *  in their payloads.
 */
class ClientNodeApp : public CheckpointApp
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    ClientNodeApp();

    ~ClientNodeApp() override;

    /**
     * \brief set the remote address and port
     * \param ip remote IP address
     * \param port remote port
     */
    void SetRemote(Address ip, uint16_t port);
    /**
     * \brief set the remote address
     * \param addr remote address
     */
    void SetRemote(Address addr);

    /**
     * \return the total bytes sent by this app
     */
    uint64_t GetTotalTx() const;

    /** 
     * Indica em quais condições esta aplicação pode criar checkpoints ou não.
     * */
    bool mayCheckpoint() override;

    /** 
     * Método abstrato. Método chamado imediatamente antes da execução de um rollback
     * para realizar algum processamento, caso seja necessário.
     * */
    void beforeRollback() override;

    /** 
     * Método abstrato. Método chamado imediatamente após a execução de um rollback
     * para realizar algum processamento, caso seja necessário.
     * */
    void afterRollback() override;

    /** 
     * Especifica como esta classe deve ser convertida em JSON (para fins de checkpoint). 
     * NÃO MEXER NA ASSINATURA DESTE MÉTODO!
     * */
    json to_json() const;
    
    /** 
     * Especifica como esta classe deve ser convertida de JSON para objeto (para fins de rollback). 
     * NÃO MEXER NA ASSINATURA DESTE MÉTODO!
    */
    void from_json(const json& j);
  
  protected:
    void configureCheckpointStrategy() override;

  private:
    
    void StartApplication() override;
    void StopApplication() override;

    /**
     * \brief Send a packet
     */
    void Send();

    /**
     * \brief Handle a packet reception.
     *
     * This function is called by lower layers.
     *
     * \param socket the socket the packet was received to.
     */
    void HandleRead(Ptr<Socket> socket);

    void logMessageReceived(Ptr<Packet> packet, uint32_t receivedSize, Address from, 
                            uint32_t currentSequenceNumber, string command, int data);

    /** Avisa aos nós interessados que este nó concluiu seu procedimento de rollback. */
    void notifyNodesAboutRollbackConcluded();

    /** 
     * Apaga os dados deste nó quando ele entra em modo SLEEP, DEPLETED ou quando ocorre
     * algum erro.
    */
    void resetNodeData();

    /** 
     * Imprime os dados dos atributos desta classe (para fins de debug).
    */
    void printNodeData();

    ////////////////////////////////////////////////
    //////          ATRIBUTOS NATIVOS         //////
    ////////////////////////////////////////////////

    /* 
      Atributos nativos não são controlados pela aplicação. Podem ser atributos físicos,
      como, por exemplo, a carga atual da bateria, ou atributos fixos (que nunca mudam) 
      de uma aplicação. São atributos que não são incluídos em checkpoints.
    */

    /// Traced Callback: transmitted packets.
    TracedCallback<Ptr<const Packet>> m_txTrace;

    /// Callbacks for tracing the packet Rx events
    TracedCallback<Ptr<const Packet>> m_rxTrace;

    /// Callbacks for tracing the packet Tx events, includes source and destination addresses
    TracedCallback<Ptr<const Packet>, const Address&, const Address&> m_txTraceWithAddresses;

    /// Callbacks for tracing the packet Rx events, includes source and destination addresses
    TracedCallback<Ptr<const Packet>, const Address&, const Address&> m_rxTraceWithAddresses;

    Ptr<Socket> m_socket;  //!< Socket
    EventId m_sendEvent;   //!< Event to send the next packet

    bool rollbackInProgress; //Indica se existe um procedimento de rollback em andamento

    ////////////////////////////////////////////////
    //////       ATRIBUTOS DE APLICAÇÃO       //////
    ////////////////////////////////////////////////

    //Somente atributos de aplicação serão armazenados em checkpoints

    uint32_t m_count; //!< Maximum number of packets the application will send
    Time m_interval;  //!< Packet inter-send time
    uint32_t m_sent;       //!< Counter for sent packets
    uint64_t last_seq;     //!< Last sequence number received from the server
    uint64_t m_totalTx;    //!< Total bytes sent
    Address m_address;     //!< This node's address
    uint16_t m_port;       //!< This node's port
    Address m_peerAddress; //!< Remote peer address
    uint16_t m_peerPort;   //!< Remote peer port
    //uint32_t m_size;  //!< Size of the sent packet (including the SeqTsHeader)
    //uint8_t m_tos;         //!< The packets Type of Service

/*#ifdef NS3_LOG_ENABLE
    std::string m_peerAddressString; //!< Remote peer address string
#endif                               // NS3_LOG_ENABLE*/

};

} // namespace ns3

#endif /* CLIENT_NODE_APP_H */
