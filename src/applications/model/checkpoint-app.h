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

#include "ns3/enum.h"
#include "ns3/battery.h"
#include "fixed-energy-generator.h"
#include "circular-energy-generator.h"
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

enum EnergyMode
{
  NORMAL,
  SLEEP,
  DEPLETED
};


class CheckpointStrategy; 
class Socket;
class Packet;

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

    //Bateria do nó
    Ptr<Battery> battery;

    /** Gerador de energia da bateria do nó */
    Ptr<EnergyGenerator> energyGenerator;

    //Modo atual do nó
    enum EnergyMode currentMode;
    
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

    int totalNodesQuantity;   //Quantidade de nós da simulação. Utilizado por alguns protocolos.

    /* Callback de interceptação do protocolo de checkpointing a ser chamado APÓS a aplicação receber 
    uma mensagem. */
    Callback<void, Ptr<MessageData>> protocolAfterReceiveCallback;

    //////////////////////////////////////////////////////////////
    //////       ATRIBUTOS ARMAZENADOS EM CHECKPOINTS       //////
    //////////////////////////////////////////////////////////////

    //Somente atributos de aplicação serão armazenados em checkpoints

    Ptr<UDPHelper> udpHelper; //Auxilia a conexão de um nó com outro
    Ptr<CheckpointStrategy> checkpointStrategy; // Estratégia de checkpoint escolhida para este nó.
    string nodeName;          //nome deste nó
    string configFilename;    //nome do arquivo de configuração

    /** Define a estratégia de checkpointing a ser utilizada por este nó. */
    virtual void configureCheckpointStrategy();

    /** 
     * Carrega as configurações do arquivo de configurações referentes a esta classe.
    */
    void loadConfigurations();

    /** Define o gerador de energia a ser utilizado por este nó. */
    void configureEnergyGenerator();

    /**
     * Realiza um processo de rollback para o último checkpoint criado, por iniciativa do 
     * próprio nó. Não reseta os dados do nó (isso já deve ter sido feito antes).
     */
    void initiateRollbackToLastCheckpoint();

    /** 
     * MÉTODO ABSTRATO. A implementação fica a cargo da classe derivada.
     * Apaga os dados deste nó quando ele entra em modo SLEEP, DEPLETED ou quando ocorre
    algum erro.
    */
    virtual void resetNodeData();

    /** Retorna se o nó possui energia suficiente para realizar determinada ação sem entrar em modo SLEEP. */
    virtual bool hasEnoughEnergy(double requiredEnergy);

     /** Diminui a energia da bateria referente ao seu funcionamento básico */
     void decreaseIdleEnergy(); 
    
     /** Diminui a energia da bateria quando em modo sleep */
     void decreaseSleepEnergy();
     
     /** Diminui a energia da bateria referente ao modo atual em que ela se encontra */
     void decreaseCurrentModeEnergy();
     
     /** Diminui a energia da bateria referente à conexão de um socket */
     void decreaseConnectEnergy();
 
     /** Método que centraliza o desconto de energia da bateria do nó. Contém o processamento principal. */
     void decreaseEnergy(double amount);
     
     /** Verifica se a bateria deve mudar de modo, com base na energia restante */
     void checkModeChange();
 
     /** Gera energia para a bateria, ou seja, a recarrega. */
     void generateEnergy();
 
     EnergyMode getCurrentMode();
     
     Time getEnergyUpdateInterval();

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

    virtual void StartApplication();

    virtual void StopApplication();

    /** 
     * Envia um pacote genérico para um nó.
     * 
     * @param command comando que indica o tipo de mensagem.
     * @param d dado que será transmitido na mensagem. 0 caso não seja necessário.
     * @param to Indica para qual endereço o pacote será enviado.
     * @param replay Indica que o envio na verdade é um replay. Não reenvia a mensagem de fato, 
     * apenas registra novamente que ela havia sido enviada.
     * @param piggyBackedInfo Informações adicionais que serão enviadas junto à mensagem.
     * @return A mensagem enviada. Nulo caso não tenha sido possível enviar.
     * */
    virtual Ptr<MessageData> send(string command, int d, Address to, bool replay = false, string piggyBackedInfo = "");

    /** 
     * Envia um pacote para um nó.
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
    virtual Ptr<MessageData> send(string command, int d, Ipv4Address ip, uint16_t port, bool replay = false, string piggyBackedInfo = "");

    /** 
     * Reenvia uma mensagem previamente enviada.
     * 
     * @param m Mensagem que será reenviada.
     * */
    Ptr<MessageData> resend(Ptr<MessageData> m);

    /** 
     * Registra o recebimento de uma mensagem que havia sido recebida previamente a uma falha.
     * 
     * @param md Mensagem que terá seu recebimento reprocessado e registrado.
     * @param replayResponse Indica se deve registrar uma possível resposta à mensagem recebida
     * (se for o caso).
     */
    virtual void replayReceive(Ptr<MessageData> md, bool replayResponse);

    /**
     * Reseta os dados do nó e realiza um processo de rollback para um checkpoint específico, 
     * quando solicitado por outro nó.
     * 
     * @param requester Nó que requisitou o rollback.
     * @param cpId ID do checkpoint para o qual será feito rollback.
     */
    virtual void initiateRollback(Address requester, int cpId, string piggyBackedInfo = "");

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
     * MÉTODO ABSTRATO. A implementação fica a cargo da classe derivada.
     * Imprime os dados dos atributos desta classe (para fins de debug). 
     * */
    virtual void printNodeData();

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

    /** Retorna se o nó possui energia suficiente para enviar um pacote sem entrar em modo SLEEP. */
    virtual bool hasEnoughEnergyToSendPacket();

    /** Retorna se o nó possui energia suficiente para receber um pacote sem entrar em modo SLEEP. */
    virtual bool hasEnoughEnergyToReceivePacket();

    /** Retorna se o nó possui energia suficiente para criar um checkpoint sem entrar em modo SLEEP. */
    virtual bool hasEnoughEnergyToCheckpoint();

    bool isSleeping();
    
    bool isDepleted();

    /** Retorna true caso não esteja dormindo nem descarregado. */
    bool isAlive();

    /** 
     * Atribui um método de callback, a ser chamado quando um pacote for recebido 
     * no socket que está sendo usado. Esse callback é referente ao protocolo que
     * estiver sendo testado na simulação. A mensagem será processada primeiro pelo
     * protocolo, e só depois pela aplicação.
     * */
    void setProtocolAfterReceiveCallback(Callback<void, Ptr<MessageData>> callback);


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
