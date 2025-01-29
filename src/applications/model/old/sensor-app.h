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

#ifndef SENSOR_APP_H
#define SENSOR_APP_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/battery.h"
#include "ns3/energy-generator.h"

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
 * \brief Aplicação referente a um nó sensor.
 */
class SensorApp : public Application
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    SensorApp();
    ~SensorApp() override;

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

    uint16_t m_port;       //!< Port on which we listen for incoming packets.
    uint8_t m_tos;         //!< The packets Type of Service
    Ptr<Socket> m_socket;  //!< IPv4 Socket
    Ptr<Socket> m_socket6; //!< IPv6 Socket
    Address m_local;       //!< local multicast address

    /// Callbacks for tracing the packet Rx events
    TracedCallback<Ptr<const Packet>> m_rxTrace;

    /// Callbacks for tracing the packet Rx events, includes source and destination addresses
    TracedCallback<Ptr<const Packet>, const Address&, const Address&> m_rxTraceWithAddresses;
    
    Battery battery;  //!< bateria do sensor
    enum Mode currentMode; //Modo atual da bateria
    double idleEnergyConsumption; //Consumo de energia em modo idle
    double sleepEnergyConsumption; //Consumo de energia em modo sleep
    Time energyUpdateInterval; //Intervalo de atualização da energia (geração e modo idle)

    /** Gerador de energia da bateria do nó */
    EnergyGenerator *energyGenerator;

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
    bool isSleeping();
    bool isDepleted();
    Time getEnergyUpdateInterval();

};

} // namespace ns3

#endif /* UDP_ECHO_SERVER_H */
