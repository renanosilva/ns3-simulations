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

#ifndef ECS_CHECKPOINT_APP_H
#define ECS_CHECKPOINT_APP_H

#include "ns3/checkpoint-app.h"
#include "ns3/enum.h"
#include "ns3/battery.h"
#include "ns3/fixed-energy-generator.h"
#include "ns3/circular-energy-generator.h"
#include "ns3/application.h"
#include "ns3/checkpoint-strategy.h"
#include "ns3/ptr.h"
#include "ns3/udp-helper.h"
#include "ns3/config-helper.h"
#include "ns3/application-type.h"
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>

using json = nlohmann::json;
using namespace std;

namespace ns3
{

class CheckpointStrategy; 
class Socket;
class Packet;

/**
 * \ingroup applications
 * \defgroup applications
 */

/**
 * \ingroup applications
 * \brief Aplicação referente a um nó que realiza checkpoint em um cenário com utilização de estação central.
 */
class ECSCheckpointApp : public CheckpointApp
{
  protected:

    /////////////////////////////////////////////////////////////////////////////
    //////          ATRIBUTOS    NÃO    ARMAZENADOS EM CHECKPOINTS         //////
    /////////////////////////////////////////////////////////////////////////////

    /* 
      Por questão de organização, aqui devem ser declarados os atributos que não devem ser armazenados em 
      checkpoints. Exemplos desses tipos de atributos incluem atributos físicos, como, a carga atual da bateria, 
      ou atributos fixos (que nunca mudam) de uma aplicação.
    */

    Ipv4Address ecsAddress; // Endereço da estação central de suporte
    uint16_t ecsPort; // Porta da estação central de suporte
    uint16_t m_port; // Porta utilizada para comunicação

    //////////////////////////////////////////////////////////////
    //////       ATRIBUTOS ARMAZENADOS EM CHECKPOINTS       //////
    //////////////////////////////////////////////////////////////

    //Somente atributos de aplicação serão armazenados em checkpoints
    
    //Sem atributos desse tipo no momento

    /** Define a estratégia de checkpointing a ser utilizada por este nó. */
    virtual void configureCheckpointStrategy() override;

    /** Método que centraliza o desconto de energia da bateria do nó. Contém o processamento principal. */
    // virtual void decreaseEnergy(double amount) override;

     /** 
     * Converte uma string em um Address.
     */
    void SetECSAddress(string address);

    /** Converte um Address em uma string. */
    string GetAddressAsString(Address a) const;

    /** Converte o endereço do ECS em uma string. */
    string GetECSAddress() const;

  public:

    /** 
     * Envia um pacote para um destinatário através da estação central.
     * 
     * @param command comando que indica o tipo de mensagem.
     * @param d dado que será transmitido na mensagem. 0 caso não seja necessário.
     * @param to Indica para qual endereço o pacote será enviado.
     * @param replay Indica que o envio na verdade é um replay. Não reenvia a mensagem de fato, 
     * apenas registra novamente que ela havia sido enviada.
     * @param piggyBackedInfo Informações adicionais que serão enviadas junto à mensagem.
     * @return A mensagem enviada. Nulo caso não tenha sido possível enviar.
     * */
    Ptr<MessageData> sendViaECS(string command, int d, Address to, bool replay = false, string piggyBackedInfo = "");

    /** 
     * Envia um pacote para um destinatário através da estação central.
     * 
     * @param command comando que indica o tipo de mensagem.
     * @param d dado que será transmitido na mensagem. 0 caso não seja necessário.
     * @param ip IP de destino.
     * @param port Porta de destino.
     * @param replay Indica que o envio na verdade é um replay. Não reenvia a mensagem de fato, 
     * apenas registra novamente que ela havia sido enviada.
     * @param piggyBackedInfo Informações adicionais que serão enviadas junto à mensagem.
     * @return A mensagem enviada. Nulo caso não tenha sido possível enviar.
     * */
    Ptr<MessageData> sendViaECS(string command, int d, Ipv4Address ip, uint16_t port, bool replay = false, 
                                        string piggyBackedInfo = "");

    /** 
     * Envia uma mensagem para a estação central.
     * 
     * @param command comando que indica o tipo de mensagem.
     * @param d dado que será transmitido na mensagem. 0 caso não seja necessário.
     * @param piggyBackedInfo Informações adicionais que serão enviadas junto à mensagem. Opcional.
     * @return A mensagem enviada. Nulo caso não tenha sido possível enviar.
     * */
    Ptr<MessageData> sendToECS(string command, int d, string piggyBackedInfo = "");

    /** 
     * Especifica como esta classe deve ser convertida em JSON (para fins de checkpoint). 
     * NÃO MEXER NA ASSINATURA DESTE MÉTODO!
     * */
    virtual json to_json() const;
    
    /** 
     * Especifica como esta classe deve ser convertida de JSON para objeto (para fins de rollback). 
     * NÃO MEXER NA ASSINATURA DESTE MÉTODO!
    */
    virtual void from_json(const json& j);

};

} // namespace ns3

#endif /* CHECKPOINT_APP_H */
