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

#ifndef CHECKPOINT_APP_H
#define CHECKPOINT_APP_H

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

/**
 * \ingroup applications
 * \defgroup applications
 */

/**
 * \ingroup applications
 * \brief Aplicação referente a um nó que realiza checkpoint.
 */
class CheckpointApp : public Application
{
  private:

    virtual void StartApplication();

    virtual void StopApplication();

  protected:

    /////////////////////////////////////////////////////////////////////////////
    //////          ATRIBUTOS    NÃO    ARMAZENADOS EM CHECKPOINTS         //////
    /////////////////////////////////////////////////////////////////////////////

    /* 
      Por questão de organização, aqui devem ser declarados os atributos que não devem ser armazenados em 
      checkpoints. Exemplos desses tipos de atributos incluem atributos físicos, como, a carga atual da bateria, 
      ou atributos fixos (que nunca mudam) de uma aplicação.
    */

    /** Indica o tipo desta aplicação. Ex.: CLIENT ou SERVER. */
    ApplicationType applicationType;

    /** Helper que auxilia no gerenciamento das configurações do nó. */
    Ptr<ConfigHelper> configHelper;

    //////////////////////////////////////////////////////////////
    //////       ATRIBUTOS ARMAZENADOS EM CHECKPOINTS       //////
    //////////////////////////////////////////////////////////////

    //Somente atributos de aplicação serão armazenados em checkpoints

    Ptr<UDPHelper> udpHelper; //Auxilia a conexão de um nó com outro
    Ptr<CheckpointStrategy> checkpointStrategy; // Estratégia de checkpoint escolhida para este nó.
    string nodeName;          //nome deste nó
    string configFilename;    //nome do arquivo de configuração

    /** Método que centraliza o desconto de energia da bateria do nó. Contém o processamento principal. */
    virtual void decreaseEnergy(double amount);

    /** Define a estratégia de checkpointing a ser utilizada por este nó. */
    virtual void configureCheckpointStrategy();

  public:

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /** Construtor padrão */
    CheckpointApp();
    
    /** Destrutor padrão */
    ~CheckpointApp() override;

    /** 
     * Envia um pacote genérico para um nó.
     * 
     * @param command comando que indica o tipo de mensagem.
     * @param d dado que será transmitido na mensagem. 0 caso não seja necessário.
     * @param to Indica para qual endereço o pacote será enviado.
     * */
    virtual Ptr<MessageData> send(string command, int d, Address to);

    /** 
     * Envia um pacote para um nó.
     * 
     * @param command comando que indica o tipo de mensagem.
     * @param d dado que será transmitido na mensagem. 0 caso não seja necessário.
     * @param ip IP de destino.
     * @param port Porta de destino.
     * */
    virtual Ptr<MessageData> send(string command, int d, Ipv4Address ip, uint16_t port);

    /**
     * Reseta os dados do nó e realiza um processo de rollback para um checkpoint específico, 
     * quando solicitado por outro nó.
     * 
     * @param requester Nó que requisitou o rollback.
     * @param cpId ID do checkpoint para o qual será feito rollback.
     */
    virtual void initiateRollback(Address requester, int cpId);

    /** 
     * Método abstrato chamado imediatamente antes do cancelamento de um checkpoint.
     * */
    virtual void beforeCheckpointDiscard();

    /** 
     * Método abstrato chamado imediatamente após o cancelamento de um checkpoint.
     * */
    virtual void afterCheckpointDiscard();

    /** 
     * Método abstrato chamado imediatamente antes da execução de um rollback
     * para realizar algum processamento, caso seja necessário.
     * */
    virtual void beforeRollback();

    /** 
     * Método abstrato chamado imediatamente após a execução de um rollback
     * para realizar algum processamento, caso seja necessário.
     * */
    virtual void afterRollback();

    /**
     * Método abstrato. Serve para indicar em quais condições uma aplicação pode criar
     * um checkpoint ou não.
     * Por padrão, uma aplicação sempre pode criar checkpoints.
     * Caso não deseje dessa forma, a aplicação deve sobrescrever este método. 
     */
    virtual bool mayCheckpoint();

    /**
     * Método abstrato. Serve para indicar em quais condições uma aplicação pode remover
     * um checkpoint ou não.
     * Por padrão, uma aplicação sempre pode remover checkpoints.
     * Caso não deseje dessa forma, a aplicação deve sobrescrever este método. 
     */
    virtual bool mayRemoveCheckpoint();

    /** Diminui a energia do nó referente ao recebimento de um pacote. */
    virtual void decreaseReadEnergy(); 
    
    /** Diminui a energia do nó referente ao envio de um pacote. */
    virtual void decreaseSendEnergy();
    
    /** Diminui a energia do nó referente à criação de um checkpoint. */
    virtual void decreaseCheckpointEnergy();

    /** Diminui a energia do nó referente ao processo de rollback. */
    virtual void decreaseRollbackEnergy();

    ApplicationType getApplicationType();

    string getNodeName();

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
