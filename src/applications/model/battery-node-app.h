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

#include "packet-loss-counter.h"

#include "ns3/battery.h"
#include "ns3/udp-helper.h"
#include "ns3/checkpoint-app.h"
#include "ns3/address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
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

enum Mode
{
  NORMAL,
  SLEEP,
  DEPLETED
};

class Socket;
class Packet;

/**
 * \ingroup applications
 * \defgroup sensor
 */

/**
 * \ingroup sensor
 * \brief Aplicação referente a um nó que funcion a bateria.
 */
class BatteryNodeApp : public CheckpointApp {

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
    
    Battery battery;  //!< bateria do sensor
    enum Mode currentMode; //Modo atual do nó
    
    Time energyUpdateInterval; //Intervalo de atualização da energia (geração e modo idle)
    double sleepEnergyConsumption; //Consumo de energia em modo sleep
    double idleEnergyConsumption; //Consumo de energia em modo idle
    double rollbackEnergyConsumption; //Consumo de energia para se realizar um rolback
    double createCheckpointConsumption; //Consumo de energia para se criar um checkpoint
    double receivePacketConsumption; //Consumo de energia para o recebimento de um pacote
    double sendPacketConsumption; //Consumo de energia para o envio de um pacote
    double connectConsumption; //Consumo de energia para conectar um socket
    double sleepModePercentage; //Porcentagem da bateria a partir da qual se entra no modo sleep
    double normalModePercentage; //Porcentagem da bateria a partir da qual se volta para o modo normal (estando no modo sleep)

    /** Gerador de energia da bateria do nó */
    Ptr<EnergyGenerator> energyGenerator;

    //////////////////////////////////////////////////////////////
    //////       ATRIBUTOS ARMAZENADOS EM CHECKPOINTS       //////
    //////////////////////////////////////////////////////////////

    //Somente atributos de aplicação serão armazenados em checkpoints

    Ptr<UDPHelper> udpHelper;       //Auxilia a conexão de um nó com outro
    uint16_t m_port;                //!< Port on which we listen for incoming packets.
    
    /** Numeração de sequência. Sempre que uma mensagem é processada, o número é incrementado. */
    uint64_t m_seq;
    
    ////////////////////////////////////////////////
    //////              MÉTODOS               //////
    ////////////////////////////////////////////////
    
    void StartApplication() override;

    void StopApplication() override;

    /**
     * \brief Handle a packet reception.
     *
     * This function is called by lower layers.
     *
     * \param socket the socket the packet was received to.
     */
    void HandleRead(Ptr<MessageData> md);

    /**
     * Gerencia a recepção de um pacote enquanto está no modo de rollback.
     *
     * \param md Dados da mensagem recebida.
     */
    void HandleReadInRollbackMode(Ptr<MessageData> md);

    /**
     * Gerencia a recepção de um pacote enquanto está no modo de criação de checkpoint.
     *
     * \param md Dados da mensagem recebida.
     */
    void HandleReadInCheckpointMode(Ptr<MessageData> md);

    /** 
     * Carrega as configurações do arquivo de configurações referentes a esta classe.
    */
    void loadConfigurations();

    /** 
     * Notifica os nós com os quais houve comunicação sobre a necessidade de realizarem rollback.
     * Método chamado quando este nó realiza seu próprio rollback.
     */
    void notifyNodesAboutRollback();

    /** 
     * Notifica os nós com os quais houve comunicação sobre a conclusão do checkpoint deste nó.
     */
    void notifyNodesAboutCheckpointConcluded();

    /** 
     * Conclui a criação de um checkpoint, após receber notificação dos outros nós dependentes,
     * ou o cancela. 
     * @param confirm Indica se o checkpoint será confirmado ou descartado.
    */
    void confirmCheckpointCreation(bool confirm);

        /**
     * Inicia um processo de rollback para um checkpoint específico.
     * @param requester Nó que requisitou o rollback.
     * @param cpId ID do checkpoint para o qual será feito rollback.
     */
    void startRollback(Address requester, int cpId);

    /** 
     * Apaga os dados deste nó quando ele entra em modo SLEEP, DEPLETED ou quando ocorre
    algum erro.
    */
    void resetNodeData();

    /** Define o gerador de energia a ser utilizado por este nó. */
    void configureEnergyGenerator();

    /** Diminui a energia da bateria referente ao seu funcionamento básico */
    void decreaseIdleEnergy(); 
    
    /** Diminui a energia da bateria quando em modo sleep */
    void decreaseSleepEnergy();
    
    /** Diminui a energia da bateria referente ao modo atual em que ela se encontra */
    void decreaseCurrentModeEnergy();
    
    /** Diminui a energia da bateria referente ao recebimento de um pacote */
    void decreaseReadEnergy(); 
    
    /** Diminui a energia da bateria referente ao envio de um pacote */
    void decreaseSendEnergy();
    
    /** Diminui a energia da bateria referente à conexão de um socket */
    void decreaseConnectEnergy();

    /** Diminui a energia da bateria referente à criação de um checkpoint */
    void decreaseCheckpointEnergy();

    /** Diminui a energia da bateria referente ao processo de rollback */
    void decreaseRollbackEnergy();

    /** Método que centraliza o desconto de energia da bateria do nó. Contém o processamento principal. */
    void decreaseEnergy(double amount);
    
    /** Verifica se a bateria deve mudar de modo, com base na energia restante */
    void checkModeChange();

    /** Gera energia para a bateria, ou seja, a recarrega. */
    void generateEnergy();

    Mode getCurrentMode();
    
    Time getEnergyUpdateInterval();

    /** 
       * Imprime os dados dos atributos desta classe (para fins de debug).
      */
    void printNodeData();

  protected:

    void configureCheckpointStrategy() override;

  public:

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    BatteryNodeApp();
    ~BatteryNodeApp() override;

    bool isSleeping();
    
    bool isDepleted();

    /** 
     * Método abstrato. Método chamado imediatamente antes da criação de um checkpoint
     * (parcial, ainda não confirmado) para realizar algum processamento, caso seja necessário.
     * */
    void beforePartialCheckpoint() override;

    /** 
     * Método abstrato. Método chamado imediatamente após a criação de um checkpoint
     * (parcial, ainda não confirmado) para realizar algum processamento, caso seja necessário.
     * */
    void afterPartialCheckpoint() override;

    /** 
     * Método chamado imediatamente após o cancelamento de um checkpoint
     * para realizar algum processamento, caso seja necessário.
     * */
    void afterCheckpointDiscard() override;

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
     * Indica em quais condições esta aplicação pode criar checkpoints ou não.
     * */
    bool mayCheckpoint() override;

    /** 
     * Indica em quais condições esta aplicação pode remover checkpoints ou não.
     * */
    bool mayRemoveCheckpoint() override;

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
