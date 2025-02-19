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

#include "ns3/checkpoint-app.h"
#include "ns3/address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/battery.h"
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
class BatteryNodeApp : public CheckpointApp
{
  public:

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    BatteryNodeApp();
    ~BatteryNodeApp() override;

    /**
     * \brief Returns the number of lost packets
     * \return the number of lost packets
     */
    uint32_t GetLost() const;

    /**
     * \brief Returns the number of received packets
     * \return the number of received packets
     */
    uint64_t GetReceived() const;

    /**
     * \brief Returns the size of the window used for checking loss.
     * \return the size of the window used for checking loss.
     */
    uint16_t GetPacketWindowSize() const;

    /**
     * \brief Set the size of the window used for checking loss. This value should
     *  be a multiple of 8
     * \param size the size of the window used for checking loss. This value should
     *  be a multiple of 8
     */
    void SetPacketWindowSize(uint16_t size);

    string getNodeName() override;

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

    /** Diminui a energia da bateria referente à criação de um checkpoint */
    void decreaseCheckpointEnergy(); 

    bool isSleeping();
    
    bool isDepleted();

    /** 
     * Método chamado imediatamente antes da criação de um checkpoint
     * para realizar algum processamento, caso seja necessário.
     * */
    void beforeCheckpoint() override;

    /** 
     * Método chamado imediatamente após a criação de um checkpoint
     * para realizar algum processamento, caso seja necessário.
     * */
    void afterCheckpoint() override;

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

  protected:
    void defineCheckpointStrategy() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * \brief Handle a packet reception.
     *
     * This function is called by lower layers.
     *
     * \param socket the socket the packet was received to.
     */
    void HandleRead(Ptr<Socket> socket);

    /** Adiciona um endereço ao vetor de addresses. Não permite elementos repetidos. */
    void addAddress(Address a);

    /** 
     * Notifica os nós com os quais houve comunicação sobre a necessidade de realizarem rollback.
     * Método chamado quando este nó realiza seu próprio rollback.
     */
    void notifyNodesAboutRollback();

    /** 
     * Apaga os dados deste nó quando ele entra em modo SLEEP, DEPLETED ou quando ocorre
    algum erro.
    */
   void resetNodeData();

   /** Define o gerador de energia a ser utilizado por este nó. */
   void defineEnergyGenerator();

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

   /** Método que centraliza o desconto de energia da bateria do nó. Contém o processamento principal. */
   void decreaseEnergy(double amount);
   
   /** Verifica se a bateria deve mudar de modo, com base na energia restante */
   void checkModeChange();

   /** Gera energia para a bateria, ou seja, a recarrega. */
   void generateEnergy();

   Mode getCurrentMode();
  
   Time getEnergyUpdateInterval();

    ////////////////////////////////////////////////
    //////          ATRIBUTOS NATIVOS         //////
    ////////////////////////////////////////////////

    /* 
      Atributos nativos não são controlados pela aplicação. Podem ser atributos físicos,
      como, por exemplo, a carga atual da bateria, ou atributos fixos (que nunca mudam) 
      de uma aplicação. Esses atributos não são incluídos em checkpoints.
    */

    /// Callbacks for tracing the packet Rx events
    TracedCallback<Ptr<const Packet>> m_rxTrace;

    /// Callbacks for tracing the packet Rx events, includes source and destination addresses
    TracedCallback<Ptr<const Packet>, const Address&, const Address&> m_rxTraceWithAddresses;
    
    Battery battery;  //!< bateria do sensor
    enum Mode currentMode; //Modo atual do nó
    double idleEnergyConsumption; //Consumo de energia em modo idle
    double sleepEnergyConsumption; //Consumo de energia em modo sleep
    Time energyUpdateInterval; //Intervalo de atualização da energia (geração e modo idle)

    /** Gerador de energia da bateria do nó */
    EnergyGenerator *energyGenerator;

    /**
     * Indica se um procedimento de rollback está em progresso. Quando um rollback
     * é iniciado, é necessário aguardar que todos os nós envolvidos o conclua para
     * que a comunicação possa ser restabelecida.
     */
    bool rollbackInProgress;
  
    ////////////////////////////////////////////////
    //////       ATRIBUTOS DE APLICAÇÃO       //////
    ////////////////////////////////////////////////

    //Somente atributos de aplicação serão armazenados em checkpoints

    Ptr<Socket> m_socket;  //!< IPv4 Socket
    //Ptr<Socket> m_socket6; //!< IPv6 Socket
    uint16_t m_port;       //!< Port on which we listen for incoming packets.
    uint8_t m_tos;         //!< The packets Type of Service
    Address m_local;       //!< local multicast address
    uint64_t m_received;             //!< Number of received packets
    uint64_t m_seq;             //!< Numeração de sequência das mensagens recebidas. É incrementada ao receber uma mensagem.
    PacketLossCounter m_lossCounter; //!< Lost packet counter

    /** Endereços dos outros nós com os quais este nó se comunicou desde o último checkpoint. */
    vector<Address> addresses;

};

} // namespace ns3

#endif /* UDP_ECHO_SERVER_H */
