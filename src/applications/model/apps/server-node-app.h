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

#ifndef BATTERY_NODE_APP_H
#define BATTERY_NODE_APP_H

#include "ns3/packet-loss-counter.h"
#include "ns3/checkpoint-app.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/energy-generator.h"
#include "ns3/json-utils.h"
#include <nlohmann/json.hpp>
#include <algorithm>

using json = nlohmann::json;
using namespace std;

namespace ns3
{

/**
 * \ingroup applications
 * \brief Aplicação referente a um nó que representa um servidor, respondendo a requisições que chegam.
 */
class ServerNodeApp : public CheckpointApp {

  private:

    /////////////////////////////////////////////////////////////////////////////
    //////          ATRIBUTOS    NÃO    ARMAZENADOS EM CHECKPOINTS         //////
    /////////////////////////////////////////////////////////////////////////////

    /* 
      Por questão de organização, aqui devem ser declarados os atributos que não devem ser armazenados em 
      checkpoints. Exemplos desses tipos de atributos incluem atributos físicos, como, a carga atual da bateria, 
      ou atributos fixos (que nunca mudam) de uma aplicação.
    */

    /// Callbacks for tracing the packet Rx events
    TracedCallback<Ptr<const Packet>> m_rxTrace;

    /// Callbacks for tracing the packet Rx events, includes source and destination addresses
    TracedCallback<Ptr<const Packet>, const Address&, const Address&> m_rxTraceWithAddresses;
    
    //////////////////////////////////////////////////////////////
    //////       ATRIBUTOS ARMAZENADOS EM CHECKPOINTS       //////
    //////////////////////////////////////////////////////////////

    //Somente atributos de aplicação serão armazenados em checkpoints

    /** Port on which we listen for incoming packets. */
    uint16_t m_port;
    
    /** Numeração de sequência. Sempre que uma mensagem é processada, o número é incrementado. */
    uint64_t m_seq;
    
    ////////////////////////////////////////////////
    //////              MÉTODOS               //////
    ////////////////////////////////////////////////
    
    /**
     * \brief Handle a packet reception.
     *
     * This function is called by lower layers.
     *
     * \param socket the socket the packet was received to.
     */
    void HandleRead(Ptr<MessageData> md);

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

    ServerNodeApp();

    ~ServerNodeApp() override;

    virtual void StartApplication();

    virtual void StopApplication();

    /** Imprime os dados dos atributos desta classe (para fins de debug). */
    virtual void printNodeData() override;

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

#endif /* UDP_ECHO_SERVER_H */
