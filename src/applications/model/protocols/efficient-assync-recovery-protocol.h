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

#ifndef EFFICIENT_ASSYNC_RECOVERY_PROTOCOL_H
#define EFFICIENT_ASSYNC_RECOVERY_PROTOCOL_H

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
#include <ns3/double.h>
#include <unordered_set>

namespace ns3
{

/** 
 * Efficient Assync Recovery Protocolo Node Info.
 * Representa informações que precisam ser armazenadas sobre outros nós. 
 * Essas informações são utilizadas pelo protocolo.
*/
class EARPNodeInfo {
  
  private:
    
    Address address; //endereço do nó
    int messagesSent; //quantidade de mensagens enviadas para o nó
    int messagesReceived; //quantidade de mensagens recebidas do nó

  public:

    EARPNodeInfo(){};
    
    EARPNodeInfo(const Address& addr)
      : address(addr) {
        messagesSent = 0;
        messagesReceived = 0;
    }

    Address GetAddress() const { return address; }
    int GetMessagesSent() const { return messagesSent; }
    int GetMessagesReceived() const { return messagesReceived; }

    void SetAddress(const Address& a) { address = a; }
    void SetMessagesSent(const int& m) { messagesSent = m; }
    void SetMessagesReceived(const int& m) { messagesReceived = m; }

    //Especifica como deve ser feita a conversão desta classe em JSON
    friend void to_json(json& j, const EARPNodeInfo& obj);

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    friend void from_json(const json& j, EARPNodeInfo& obj);
};

/** 
 * Representa um evento, que pode ser o envio ou recebimento de uma mensagem.
 * Um evento fica armazenado na memória volátil do nó até que, de tempos em tempos,
 * é gravado no armazenamento estável, o que representa a criação do checkpoint.
 * Um evento armazena informações cruciais para o rastreamento das mudanças
 * de estados pelas quais o nó passa.
 * 
*/
class Event : public Object {
  
  private:
    
    int j; //índice do evento
    json nodeState; //estado do nó antes do evento
    MessageData m; //mensagem que originou o evento
    vector<MessageData> mSent; //conjunto de mensagens enviadas em resposta ao evento
    
  public:

    Event(){};
    
    int GetJ() const { return j; }
    json GetNodeState() const { return nodeState; }
    MessageData GetMessage() const { return m; }
    vector<MessageData> GetMSent() const { return mSent; }

    void SetJ(const int& a) { j = a; }
    void SetNodeState(const json& s) { nodeState = s; }
    void SetMessage(const MessageData& message) { m = message; }
    void SetMSent(const vector<MessageData>& messages) { mSent = messages; }

    //Especifica como deve ser feita a conversão desta classe em JSON
    // friend void to_json(json& j, const Event& obj);

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    // friend void from_json(const json& j, Event& obj);

    //Especifica como deve ser feita a conversão desta classe em JSON
    friend void to_json(json& j, const Ptr<Event>& obj);

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    friend void from_json(const json& j, Ptr<Event>& obj);
};

/** 
 * Estratégia de checkpointing assíncrona, na qual são gerados checkpoints 
 * a cada determinado período de tempo, que podem ser diferentes entre cada nó.
 * A estratégia de rollback é mais eficiente do que a utilizada no Protocolo
 * de Recuperação Decentralizada. 
 */
class EfficientAssyncRecoveryProtocol : public CheckpointStrategy {

  private:

    //////////////////////////////////////////////////////////////
    //////       ATRIBUTOS ARMAZENADOS EM CHECKPOINTS       //////
    //////////////////////////////////////////////////////////////

    /** 
     * Armazena os eventos ocorridos no nó (desde o último checkpoint).
     * Representa os eventos que estão armazenados em memória volátil
     * e que posteriormente é gravado na memória permanente.
     */
    vector<Ptr<Event>> events;

    /** 
     * Utilizado para armazenar informações sobre outros nós com os quais este nó
     * se comunica, como seus endereços e quantidade de mensagens enviadas
     * e recebidas. 
     * Não devem ser adicionados elementos repetidos.
     * De acordo com o protocolo original, não seria obrigatório armazenar essas
     * informações (elas poderiam ser calculadas), porém, por questões de praticidade,
     * resolvi fazer dessa forma.
     */
    vector<EARPNodeInfo> adjacentNodes;

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
     * Indica a quantidade de iterações realizadas no procedimento de rollback atual. 
     * A quantidade máxima de iterações é a quantidade de nós do sistema.
     * */
    int rollbackIteration;

    /** 
     * Mensagens de rollback recebidas na iteração atual. 
     * Representa uma fila de processamento.
     * Para passar à próxima iteração, é necessário ter recebido uma mensagem
     * de cada nó adjacente. Somente após o recebimento de todas as mensagens,
     * é realizado os seus processamentos, um a um.  
     * */
    vector<Ptr<MessageData>> rollbackMsgsToProcess;

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
    void addToAdjacentNodes(EARPNodeInfo i);

    /** 
     * Busca um objeto EARPNodeInfo na lista de nós adjacentes que possua um determinado endereço. 
     * @param addr Endereço que se está buscando na propList.
    */
    EARPNodeInfo* findAdjacentNodeByAddress(const Address& addr);

    /** 
     * Busca um objeto EARPNodeInfo que possua um determinado endereço em um vetor de EARPNodeInfo. 
     * @param v Vetor onde será feita a busca.
     * @param addr Endereço que se está buscando na propList.
    */
    EARPNodeInfo* findAdjacentNodeByAddress(vector<EARPNodeInfo>* v, const Address& addr);

    /** Reexecuta um determinado evento, após a restauração do estado da aplicação associado a ele. */
    void replayEvent(Ptr<Event> e);

    /** Processamentos realizados pelo protocolo quando uma mensagem é recebida. */
    void processMessageReceived(Ptr<MessageData> md);

    /** Processamentos realizados pelo protocolo quando uma mensagem é enviada. */
    void processMessageSent(Ptr<MessageData> md);

    /** 
     * Realiza um rollback para o último evento registrado na memória volátil.
     * Utilizado quando este nó descobre a falha de outro nó.
     */
    void rollbackToLastVolatileEvent();

    /** 
     * Realiza um rollback para um evento específico.
     * @param e Evento para o qual será feito rollback.
     * @param notifyNodes Indica se os demais nós devem ser notificados sobre o rollback.
     */
    void rollbackToVolatileEvent(Ptr<Event> e, bool notifyNodes);

    /** Processa as mensagens de rollback presentes na fila de processamento. */
    void processRollbackMsgsQueue();

    /** Indica se foram recebidas mensagens com dados de rollback de todos os nós adjacentes. */
    bool allRollbackMsgReceived();

    /** 
     * Notifica todos os nós com os quais houve comunicação sobre a necessidade de realizarem rollback,
     * exceto o que for passado por parâmetro.
     */
    void notifyNodesAboutRollbackExcept(EARPNodeInfo exceptNode);

  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /** 
     * Construtor. 
     * @param interval Define a periodicidade (em segundos) na qual serão criados checkpoints.
     * @param application Aplicação que está sendo executada no nó. Os dados dessa classe serão armazenados em checkpoint.
     * 
     * */
    EfficientAssyncRecoveryProtocol(Time timeInterval, int totalNodesQuantity, Ptr<CheckpointApp> application);

    /** Construtor padrão. */
    EfficientAssyncRecoveryProtocol();

    ~EfficientAssyncRecoveryProtocol() override;

    virtual void startCheckpointing() override;
    
    virtual void stopCheckpointing() override;

    virtual void writeCheckpoint() override; 

    /** 
     * Método utilizado para realizar um rollback para o último checkpoint válido após a recuperação
     * de uma falha do próprio nó. Realiza as iterações necessárias para garantir o rollback dos
     * nós adjacentes.
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
     * Intercepta o envio de um pacote. Dessa forma, a estratégia de checkpoint
     * tem a oportunidade analisar pacotes enviados pela aplicação.
     *
     * \param md Dados da mensagem enviada.
     * \return Retorna true caso a mensagem seja interceptada e processada pela estratégia de checkpoint. 
     * Nesse caso, a aplicação não deve processá-la. Retorna false caso contrário, ou seja, se a mensagem
     * não tiver sido processada, o que deverá ser feito pela aplicação.
     */
    virtual bool interceptSend(Ptr<MessageData> md) override;

    /** 
     * Notifica todos os nós com os quais houve comunicação sobre a necessidade de realizarem rollback.
     * Método chamado quando este nó falha e se recupera.
     */
    void notifyNodesAboutRollback();

    virtual void printData() override;

    /** Retorna o evento ativo (último evento) deste nó. */
    Ptr<Event> getActiveEvent();

    //Especifica como deve ser feita a conversão desta classe em JSON
    virtual json to_json() override;

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    virtual void from_json(const json& j) override;
};

} // namespace ns3

#endif /* EFFICIENT_ASSYNC_RECOVERY_PROTOCOL_H */
