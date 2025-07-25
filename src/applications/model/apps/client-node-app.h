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
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/checkpoint-app.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace ns3
{

class Socket;
class Packet;

/**
 * \ingroup applications
 * \brief Aplicação referente a um nó que representa um cliente, que envia periodicamente
 * requisições a um ou mais servidores.
 */
class ClientNodeApp : public CheckpointApp
{
  private:

    /////////////////////////////////////////////////////////////////////////////
    //////          ATRIBUTOS    NÃO    ARMAZENADOS EM CHECKPOINTS         //////
    /////////////////////////////////////////////////////////////////////////////

    /* 
      Por questão de organização, aqui devem ser declarados os atributos que não devem ser armazenados em 
      checkpoints. Exemplos desses tipos de atributos incluem atributos físicos, como, a carga atual da bateria, 
      ou atributos fixos (que nunca mudam) de uma aplicação.
    */

    EventId m_sendEvent;   //!< Event to send the next packet

    //////////////////////////////////////////////////////////////
    //////       ATRIBUTOS ARMAZENADOS EM CHECKPOINTS       //////
    //////////////////////////////////////////////////////////////

    //Somente atributos de aplicação serão armazenados em checkpoints

    //!< Remote peer IP addresses. Packets will be sent to these addresses.
    vector<Ipv4Address> m_peerAddresses;

    //!< Remote peer port
    uint16_t m_peerPort;
    
    //!< Maximum number of messages the application will send
    uint32_t m_count;
    
    //!< Message inter-send time
    Time m_interval;
    
    //!< Last sequence number received from the server
    uint64_t last_seq;

    /** Envia um pacote para os servidores. */
    void Send();

    /**
     * Método responsável pelo recebimento de pacotes. 
     * @param md Dados da mensagem recebida.
     */
    void HandleRead(Ptr<MessageData> md);

    void scheduleSendEvent();

  protected:

    /** 
     * Apaga os dados deste nó quando ele entra em modo SLEEP, DEPLETED ou quando ocorre
    algum erro.
    */
    virtual void resetNodeData() override;

  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    ClientNodeApp();

    ~ClientNodeApp() override;

    void StartApplication() override;

    void StopApplication() override;

    /** 
     * Registra o recebimento de uma mensagem que havia sido recebida previamente a uma falha.
     * 
     * @param md Mensagem que terá seu recebimento reprocessado e registrado.
     * @param replayResponse Indica se deve registrar uma possível resposta à mensagem recebida
     * (se for o caso).
     * @param resendResponse Indica se a mensagem de resposta deve ser de fato reenviada (se for
     * o caso de haver resposta).
     */
    virtual void replayReceive(Ptr<MessageData> md, bool replayResponse, bool resendResponse) override;

    /**
     * \return the total bytes sent by this app
     */
    uint64_t GetTotalTx() const;

    /** Imprime os dados dos atributos desta classe (para fins de debug). */
    virtual void printNodeData() override;

    /** 
     * Converte uma string padronizada no formato 'IP;IP;IP...', adicionando os endereços
     * ao vetor de endereços a enviar pacotes.
     */
    void SetPeerAddresses(string addressList);

    /** Converte o vetor de endereços em uma string. */
    string GetPeerAddresses() const;

    uint16_t getPeerPort();

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
};

} // namespace ns3

#endif /* CLIENT_NODE_APP_H */
