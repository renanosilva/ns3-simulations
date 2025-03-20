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
#include "ns3/udp-helper.h"
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
    void HandleRead(Ptr<MessageData> md);

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
      Atributos nativos são aqueles que não são armazenados em checkpoints. Podem ser 
      atributos físicos, como, por exemplo, a carga atual da bateria, ou atributos fixos 
      (que nunca mudam) de uma aplicação.
    */

    EventId m_sendEvent;   //!< Event to send the next packet
    bool rollbackInProgress; //Indica se existe um procedimento de rollback em andamento

    ////////////////////////////////////////////////
    //////       ATRIBUTOS DE APLICAÇÃO       //////
    ////////////////////////////////////////////////

    //Somente atributos de aplicação serão armazenados em checkpoints

    Ptr<UDPHelper> udpHelper; //Auxilia a conexão de um nó com outro
    Address m_peerAddress; //!< Remote peer address
    uint16_t m_peerPort;   //!< Remote peer port
    uint32_t m_count;      //!< Maximum number of messages the application will send
    Time m_interval;       //!< Message inter-send time
    uint64_t last_seq;     //!< Last sequence number received from the server
    
};

} // namespace ns3

#endif /* CLIENT_NODE_APP_H */
