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

#ifndef EARP_WITHOUT_ROLLBACK_V2_H
#define EARP_WITHOUT_ROLLBACK_V2_H

#include "ns3/object.h"
#include "ns3/core-module.h"
#include "ns3/ptr.h"
#include "ns3/type-id.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "checkpoint-strategy.h"
#include "ns3/checkpoint-helper.h"
#include "ns3/message-data.h"
#include "ns3/client-node-app.h"
#include "ns3/no-rollback-1.h"
#include <ns3/double.h>
#include <unordered_set>

namespace ns3
{

/** 
 * Representa um evento, que pode ser o envio ou recebimento de uma mensagem.
 * Um evento fica armazenado na memória volátil do nó até que, de tempos em tempos,
 * é gravado no armazenamento estável, o que representa a criação do checkpoint.
 * Um evento armazena informações cruciais para o rastreamento das mudanças
 * de estados pelos quais o nó passa.
 * 
*/
class EventEARPWRv2 : public Object {
  
  private:
    
    int j; //índice do evento
    json nodeState; //estado do nó antes do evento
    Ptr<MessageData> m; //mensagem que originou o evento
    vector<Ptr<MessageData>> mSent; //conjunto de mensagens enviadas em resposta ao evento
    
  public:

    EventEARPWRv2(){};

    ~EventEARPWRv2(){
      m = nullptr;
      mSent.clear();
    }
    
    int GetJ() const { return j; }
    json GetNodeState() const { return nodeState; }
    Ptr<MessageData> GetMessage() const { return m; }
    vector<Ptr<MessageData>> GetMSent() const { return mSent; }

    void SetJ(const int& a) { j = a; }
    void SetNodeState(const json& s) { nodeState = s; }
    void SetMessage(const Ptr<MessageData>& message) { m = message; }
    void SetMSent(const vector<Ptr<MessageData>>& messages) { mSent = messages; }

    //Especifica como deve ser feita a conversão desta classe em JSON
    friend void to_json(json& j, const Ptr<EventEARPWRv2>& obj);

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    friend void from_json(const json& j, Ptr<EventEARPWRv2>& obj);
};

/** 
 * Efficient Asynchronous Recovery Protocol Without Rollback v2 (EARP - Protocolo 
 * Eficiente de Recuperação sem Rollback versão 2).
 * 
 * Estratégia de checkpointing assíncrona, na qual são gerados checkpoints 
 * a cada determinado período de tempo, que podem ser diferentes entre cada nó.
 * A estratégia de rollback é mais eficiente do que a utilizada no Protocolo
 * de Recuperação Decentralizada. Nessa versão do protocolo EARP, somente o nó
 * que falhou precisa fazer rollback, sem limite de nós que falharam no sistema.
 */
class EARPWithoutRollbackV2 : public CheckpointStrategy {

  private:

    //////////////////////////////////////////////////////////////
    //////       ATRIBUTOS ARMAZENADOS EM CHECKPOINTS       //////
    //////////////////////////////////////////////////////////////

    /** 
     * Armazena os eventos ocorridos no nó (desde o último checkpoint).
     * Representa os eventos que estão armazenados em memória volátil
     * e que posteriormente são gravados na memória permanente.
     */
    vector<Ptr<EventEARPWRv2>> events;

    /** 
     * Utilizado para armazenar informações sobre outros nós com os quais este nó
     * se comunica, como seus endereços e quantidade de mensagens enviadas
     * e recebidas. 
     * Não devem ser adicionados elementos repetidos.
     */
    vector<Ptr<NodeInfoEARPWR>> adjacentNodes;

    /** 
     * Vetor que contém os IDs (endereços) dos nós que enviaram mensagens para este nó,
     * em ordem de envio de mensagens.
     */
    vector<Address> senderIds;

    /** Contador de eventos. */
    int eventCount;

    /** Total de nós do sistema. */
    int totalNodesQuantity;

    /////////////////////////////////////////////////////////////////////////////
    //////          ATRIBUTOS    NÃO    ARMAZENADOS EM CHECKPOINTS         //////
    /////////////////////////////////////////////////////////////////////////////

    /** 
     * Índice de recuperação ativo no momento. Refere-se à numeração do evento
     * para o qual foi feito rollback durante uma recuperação de falha. 
     */
    int rec_i;

    /** Contador de checkpoints */
    int csn;

    /** 
     * Vetor que contém os IDs (endereços) dos nós que devem reenviar mensagens
     * para este nó.
     */
    vector<Address> outIds;

    /** 
     * Fila de mensagens que devem ser reexecutadas após uma recuperação. 
     * Representa uma fila de processamento de mensagens que foram reenviadas
     * por outros nós.
     * Equivalente no algoritmo original a MSG_QUEUEi(Pj).
     * Chave: endereço do emissor.
     * Valor: mensagens reenviadas.
     * */
    map<Address, vector<Ptr<MessageData>>> resentMsgsMap;

    /** 
     * Nós que estão em falha, de acordo com o que este nó sabe.
     * Utilizado para saber se, durante uma recuperação, uma
     * mensagem precisa ser reenviada para outro nó que falhou.
     * Chave: endereço do nó remoto que falhou.
     * Valor: quantidade de mensagens que o nó remoto alega ter recebido deste nó.
     */
    map<Address, int> failedNodes;

    /** Intervalo de tempo no qual serão criados checkpoints. */
    Time interval;

    /** Evento utilizado para criação de checkpoint. */
    EventId creationScheduling;

    /** Calcula a quantidade de segundos restantes até o próximo checkpoint. */
    Time getDelayToNextCheckpoint();

    /** Agenda a criação do próximo checkpoint. */
    void scheduleNextCheckpoint();

    /** 
     * Adiciona um novo elemento à lista de nós adjacentes.
     * Não permite elementos repetidos. A adição só é feita
     * caso não exista nenhum elemento com o endereço que
     * se está tentando adicionar.
     * 
     * @param i Informações que se está tentando inserir na propList.
     */
    void addToAdjacentNodes(Address a);

    /** 
     * Busca um objeto NodeInfoEARPWR na lista de nós adjacentes que possua um determinado endereço. 
     * @param addr Endereço que se está buscando na propList.
    */
    Ptr<NodeInfoEARPWR> findAdjacentNodeByAddress(Address addr);

    /** 
     * Busca um objeto NodeInfoEARPWR que possua um determinado endereço em um vetor de NodeInfoEARPWR. 
     * @param v Vetor onde será feita a busca.
     * @param addr Endereço que se está buscando na propList.
    */
    Ptr<NodeInfoEARPWR> findAdjacentNodeByAddress(vector<Ptr<NodeInfoEARPWR>> v, Address addr);

    /** Reexecuta um determinado evento, após a restauração do estado da aplicação associado a ele. */
    void replayEvent(Ptr<EventEARPWRv2> e);

    /** Atualiza informações sobre quantidade de mensagens recebidas do nó emissor. */
    void updateNodeInfoAfterReceive(Ptr<MessageData> md);

    /** Atualiza informações sobre quantidade de mensagens enviadas para o nó receptor. */
    void updateNodeInfoAfterSend(Ptr<MessageData> md);

    /** 
     * Realiza um broadcast para verificar quantos nós estão vivos no momento.
     */
    // void broadcastCheckAlive();

    /** Intercepta o recebimento de uma mensagem de requisição de valor. */
    bool interceptRequestValueReceive(Ptr<MessageData> md);

    /** Intercepta o recebimento de uma mensagem de resposta de valor. */
    bool interceptResponseValueReceive(Ptr<MessageData> md);

    /** Realiza um processamento após o recebimento de uma mensagem de requisição de reenvio de mensagens. */
    void afterRequestResendReceive(Ptr<MessageData> md);

    /** Realiza um processamento após o recebimento de uma mensagem reenviada. */
    void afterResendMessageReceive(Ptr<MessageData> md);

    /**
     * Método chamado após o recebimento e processamento de uma mensagem pela aplicação.
     * \param md Dados da mensagem recebida.
     */
    // void afterCheckAliveReceive(Ptr<MessageData> md);

    /** Solicita o reenvio de mensagens a todos os nós adjacentes. */
    void requestResendToAdjacentNodes();

    /** Solicita o reenvio de mensagens a um nó específico. */
    void requestResend(Ptr<NodeInfoEARPWR> i);

    /** 
     * Procura pelo primeiro evento deste nó no qual o número de mensagens enviadas
     * para um nó remoto seja igual a determinado valor. Faz a busca primeiramente na memória
     * volátil. Caso não seja encontrado um evento, é feita uma busca nos checkpoints.
     *
     * @param to endereço do nó para o qual as mensagens foram enviadas.
     * @param msgsSent quantidade de mensagens que foram enviadas para o nó no evento buscado.  
     * @return evento mais recente que satisfaça os critérios de busca.
     */
    Ptr<EventEARPWRv2> searchForEvent(Address to, int msgsSent);

    /** 
     * Procura pelo primeiro evento em um vetor no qual o número de mensagens enviadas
     * para um nó remoto seja igual a determinado valor.
     *
     * @param events vetor no qual será feita a busca.
     * @param to endereço do nó para o qual as mensagens foram enviadas.
     * @param msgsSent quantidade de mensagens que foram enviadas para o nó no evento buscado.  
     * @return evento mais recente que satisfaça os critérios de busca.
     */
    Ptr<EventEARPWRv2> searchForEvent(vector<Ptr<EventEARPWRv2>> evts, Address to, int msgsSent);

    /** 
     * Procura na memória volátil e em checkpoints por mensagens a serem reenviadas.
     *
     * @param e evento a partir do qual as mensagens serão reenviadas.
     * @param to endereço do nó para o qual as mensagens devem ser reenviadas.
     * @return vetor com as mensagens a serem reenviadas.
     */
    vector<Ptr<MessageData>> searchForMsgsToResend(Ptr<EventEARPWRv2> e, Address to);

    /** 
     * Procura, em um vetor de eventos, por mensagens que devem ser reenviadas.
     *
     * @param events vetor onde será feita a busca por mensagens a reenviar. 
     * @param e evento a partir do qual as mensagens serão reenviadas.
     * @param to endereço do nó para o qual as mensagens devem ser reenviadas.
     * @return vetor com as mensagens a serem reenviadas.
     */
    vector<Ptr<MessageData>> searchForMsgsToResend(vector<Ptr<EventEARPWRv2>> events, Ptr<EventEARPWRv2> e, Address to);

    /** Reprocessa as mensagens de REQUEST ou RESPONSE que foram reenviadas. */
    void reprocessMessage(Ptr<MessageData> md);

    /** Reprocessa as mensagens de REQUEST que foram reenviadas. */
    void reprocessRequestMessage(Ptr<MessageData> md);

    /** Reprocessa as mensagens de RESPONSE que foram reenviadas.*/
    void reprocessResponseMessage(Ptr<MessageData> md);

    /** Registra no armazenamento estável o ID do emissor de uma mensagem recebida por este nó. */
    void logSenderId(Ptr<MessageData> md);

    /** Retorna o nome do arquivo que contém os IDs dos emissores de mensagens para este nó. */
    string getSenderIdsLogName();

    /** Retorna o número total de mensagens recebidas por este nó. */
    int GetTotalMsgsReceived();

    /** 
     * Método utilizado por um cliente para verificar se o servidor de destino falhou ou não, baseado
     * no recebimento ou não de uma resposta à última requisição.
      */
    bool checkIfDestinationHasFailed(Address dest);

  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /** 
     * Construtor. 
     * @param interval Define a periodicidade (em segundos) na qual serão criados checkpoints.
     * @param totalNodesQuantity Quantidade total de nós do sistema.
     * @param application Aplicação que está sendo executada no nó. Os dados dessa classe serão armazenados em checkpoint.
     * 
     * */
    EARPWithoutRollbackV2(Time timeInterval, int totalNodesQuantity, Ptr<CheckpointApp> application);

    /** Construtor padrão. */
    EARPWithoutRollbackV2();

    ~EARPWithoutRollbackV2() override;

    /** Remove as referências armazenadas na classe. */
    virtual void DisposeReferences() override;

    virtual void startCheckpointing() override;
    
    virtual void stopCheckpointing() override;

    virtual void writeCheckpoint() override; 

    /** 
     * Método utilizado para realizar um rollback para o último checkpoint válido após a recuperação
     * de uma falha do próprio nó.
     */
    virtual void rollbackToLastCheckpoint() override; 

    /** Realiza um rollback para um checkpoint específico. */
    virtual bool rollback(int cpId) override;

    /**
     * Intercepta a leitura de um pacote. Dessa forma, a estratégia de checkpoint
     * tem a oportunidade de processar a leitura antes da aplicação.
     *
     * \param md Dados da mensagem recebida.
     * \return Retorna true caso a mensagem seja interceptada e processada pela estratégia de checkpoint. 
     * Nesse caso, a aplicação não deve mais processá-la. Retorna false caso contrário, ou seja, se a 
     * mensagem não tiver sido processada, o que deverá ser feito pela aplicação.
     */
    virtual bool interceptRead(Ptr<MessageData> md) override;

    /**
     * Método chamado após o recebimento e processamento de uma mensagem pela aplicação.
     *
     * \param md Dados da mensagem recebida.
     */
    virtual void afterMessageReceive(Ptr<MessageData> md) override;

    /**
     * Intercepta o envio de um pacote. Dessa forma, a estratégia de checkpoint
     * tem a oportunidade analisar pacotes enviados pela aplicação.
     *
     * \param md Dados da mensagem enviada.
     * \return Retorna true caso a mensagem seja interceptada e processada pela estratégia de checkpoint. 
     * Nesse caso, a aplicação não deve processá-la. Retorna false caso contrário, ou seja, se a mensagem
     * não tiver sido processada, o que deverá ser feito pela aplicação.
     */
    virtual bool interceptSend(Ptr<MessageData> md) override;

    virtual void printData() override;

    /** Retorna o evento ativo (último evento) deste nó. */
    Ptr<EventEARPWRv2> getActiveEvent();

    //Especifica como deve ser feita a conversão desta classe em JSON
    virtual json to_json() override;

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    virtual void from_json(const json& j) override;

};

} // namespace ns3

#endif /* EARP_WITHOUT_ROLLBACK_V2_H */
