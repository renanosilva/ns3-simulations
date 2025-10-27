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

#ifndef EARP_WITHOUT_ROLLBACK_H
#define EARP_WITHOUT_ROLLBACK_H

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
class NodeInfoEARPWR : public Object {
  
  private:
    
    Address address; //endereço do nó
    int messagesSent; //quantidade de mensagens enviadas para o nó
    int messagesReceived; //quantidade de mensagens recebidas do nó

  public:

    NodeInfoEARPWR(){};
    
    NodeInfoEARPWR(Address addr)
      : address(addr) {
        messagesSent = 0;
        messagesReceived = 0;
    }

    Address GetAddress() const { return address; }
    int GetMessagesSent() const { return messagesSent; }
    int GetMessagesReceived() const { return messagesReceived; }

    void SetAddress(Address a) { address = a; }
    void SetMessagesSent(const int& m) { messagesSent = m; }
    void SetMessagesReceived(const int& m) { messagesReceived = m; }

    //Especifica como deve ser feita a conversão desta classe em JSON
    friend void to_json(json& j, const Ptr<NodeInfoEARPWR>& obj);

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    friend void from_json(const json& j, Ptr<NodeInfoEARPWR>& obj);

    // void to_json(json& j, const vector<Ptr<NodeInfoEARPWR>>& vec);

    // void from_json(const json& j, vector<Ptr<NodeInfoEARPWR>>& vec);
};

/** 
 * Representa um evento, que pode ser o envio ou recebimento de uma mensagem.
 * Um evento fica armazenado na memória volátil do nó até que, de tempos em tempos,
 * é gravado no armazenamento estável, o que representa a criação do checkpoint.
 * Um evento armazena informações cruciais para o rastreamento das mudanças
 * de estados pelos quais o nó passa.
 * 
*/
class EventEARPWR : public Object {
  
  private:
    
    int j; //índice do evento
    json nodeState; //estado do nó antes do evento
    Ptr<MessageData> m; //mensagem que originou o evento
    vector<Ptr<MessageData>> mSent; //conjunto de mensagens enviadas em resposta ao evento
    bool received; //indica que o evento foi gerado a partir de uma mensagem de aplicação recebida
    bool acked; //indica que a mensagem enviada do evento teve seu recebimento confirmado
    int remoteEventIndex; //indica o índice de evento atual do nó com o qual houve comunicação
    
  public:

    EventEARPWR(){};

    ~EventEARPWR(){
      m = nullptr;
      mSent.clear();
    }
    
    int GetJ() const { return j; }
    json GetNodeState() const { return nodeState; }
    Ptr<MessageData> GetMessage() const { return m; }
    vector<Ptr<MessageData>> GetMSent() const { return mSent; }
    bool IsReceived() const { return received; }
    bool IsAcked() const { return acked; }
    int GetRemoteEventIndex() const { return remoteEventIndex; }

    void SetJ(const int& a) { j = a; }
    void SetNodeState(const json& s) { nodeState = s; }
    void SetMessage(const Ptr<MessageData>& message) { m = message; }
    void SetMSent(const vector<Ptr<MessageData>>& messages) { mSent = messages; }
    void SetReceived(const bool& r) { received = r; }
    void SetAcked(const bool& a) { acked = a; }
    void SetRemoteEventIndex(const int& i) { remoteEventIndex = i; }

    //Especifica como deve ser feita a conversão desta classe em JSON
    // friend void to_json(json& j, const EventEARPWR& obj);

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    // friend void from_json(const json& j, EventEARPWR& obj);

    //Especifica como deve ser feita a conversão desta classe em JSON
    friend void to_json(json& j, const Ptr<EventEARPWR>& obj);

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    friend void from_json(const json& j, Ptr<EventEARPWR>& obj);

    // void to_json(json& j, const vector<Ptr<MessageData>>& vec);

    // void from_json(const json& j, vector<Ptr<MessageData>>& vec);
};

/** 
 * Efficient Asynchronous Recovery Protocol Without Rollback (EARP - Protocolo 
 * Eficiente de Recuperação sem Rollback).
 * 
 * Estratégia de checkpointing assíncrona, na qual são gerados checkpoints 
 * a cada determinado período de tempo, que podem ser diferentes entre cada nó.
 * A estratégia de rollback é mais eficiente do que a utilizada no Protocolo
 * de Recuperação Decentralizada. Nessa versão do protocolo EARP, somente o nó
 * que falhou precisa fazer rollback, desde que no máximo dois nós adjacentes 
 * tenham falhado em todo o sistema.
 */
class EARPWithoutRollback : public CheckpointStrategy {

  private:

    //////////////////////////////////////////////////////////////
    //////       ATRIBUTOS ARMAZENADOS EM CHECKPOINTS       //////
    //////////////////////////////////////////////////////////////

    /** 
     * Armazena os eventos ocorridos no nó (desde o último checkpoint).
     * Representa os eventos que estão armazenados em memória volátil
     * e que posteriormente é gravado na memória permanente.
     */
    vector<Ptr<EventEARPWR>> events;

    /** 
     * Utilizado para armazenar informações sobre outros nós com os quais este nó
     * se comunica, como seus endereços e quantidade de mensagens enviadas
     * e recebidas. 
     * Não devem ser adicionados elementos repetidos.
     * De acordo com o protocolo original, não seria obrigatório armazenar essas
     * informações (elas poderiam ser calculadas), porém, por questões de praticidade,
     * resolvi fazer dessa forma.
     */
    vector<Ptr<NodeInfoEARPWR>> adjacentNodes;

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
     * Nós que estão vivos, de acordo com o que este nó sabe.
     * Utilizado no início de um procedimento de rollback.
     */
    vector<Address> aliveNodes;

    /** 
     * Indica se o nó está na fase 2 do processo de rollback. A fase 2 ocorre após
     * a confirmação de que no máximo um nó adjacente falhou.
     */
    bool rollbackPhase2InProgress;

    /** 
     * Mapa que armazena a maior numeração de evento deste nó que cada nó vizinho conhece.
     * Utilizado durante rollback.
     * */
    map<Address, int> nodesMaxEventsMap;

    /** 
     * Mapa que armazena a quantidade de mensagens que cada nó deve reenviar.
     * Essas quantidades são informadas pelos próprios nós. 
     * Utilizado durante rollback.
     * */
    map<Address, int> expectedResentMsgsQuantityMap;

    /** 
     * Fila de mensagens que devem ser reexecutadas após uma recuperação. 
     * Representa uma fila de processamento de mensagens que foram reenviadas
     * por outros nós.
     * */
    map<int, Ptr<MessageData>> replayMsgsMap;

    /** 
     * Buffer de mensagens de aplicação recebidas enquanto o nó está bloqueado
     * durante uma recuperação. 
     * Representa uma fila de processamento.
     * */
    vector<Ptr<MessageData>> newAppMsgsBuffer;

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

    /** 
     * Retorna o evento mais recente que possua uma mensagem enviada a um determinado endereço e que ainda não 
     * possua confirmação de recebimento.
    */
    Ptr<EventEARPWR> getNewestEventWithoutAck(Address addr);

    /** Reexecuta um determinado evento, após a restauração do estado da aplicação associado a ele. */
    void replayEvent(Ptr<EventEARPWR> e);

    /** Processamentos realizados pelo protocolo quando uma mensagem é recebida. */
    void processMessageReceived(Ptr<MessageData> md);

    /** Processamentos realizados pelo protocolo quando uma mensagem é enviada. */
    void processMessageSent(Ptr<MessageData> md);

    /** 
     * Realiza um broadcast para verificar quantos nós estão vivos no momento. Aguarda respostas até
     * atingir um timeout.
     */
    void broadcastCheckAlive();

    /** Intercepta o recebimento de uma mensagem de requisição de valor. */
    bool interceptRequestValueReceive(Ptr<MessageData> md, bool replay = false);

    /** Intercepta o recebimento de uma mensagem de resposta de valor. */
    bool interceptResponseValueReceive(Ptr<MessageData> md, bool replay = false);

    /** Intercepta o recebimento de uma mensagem de ack. */
    bool interceptAckReceive(Ptr<MessageData> md);

    /** Realiza um processamento após o recebimento de uma mensagem de resposta de disponbilidade. */
    void afterIAmAliveReceive(Ptr<MessageData> md);

    /** Realiza um processamento após o recebimento de uma mensagem de requisição de reenvio de mensagens. */
    void afterRequestResendReceive(Ptr<MessageData> md);

    /** Realiza um processamento após o recebimento de uma mensagem de resposta de reenvio de mensagem. */
    void afterResendResponseReceive(Ptr<MessageData> md);

    /** Realiza um processamento após o recebimento de uma mensagem reenviada. */
    void afterResendMessageReceive(Ptr<MessageData> md);

    /**
     * Método chamado após o recebimento e processamento de uma mensagem pela aplicação.
     * \param md Dados da mensagem recebida.
     */
    void afterCheckAliveReceive(Ptr<MessageData> md);

    /** Solicita o reenvio de mensagens a um nó específico. */
    void requestResend(Address to);

    /** Solicita o reenvio de mensagens a todos os nós que se sabe que estão vivos. */
    void requestResendToAliveNodes();

    /** 
     * Procura por mensagens que devem ser reenviadas.
     *
     * @param v vetor onde será feita a busca por mensagens a reenviar. 
     * @param a endereço do nó para o qual as mensagens devem ser reenviadas.
     * @param j índice de evento do nó remoto a partir do qual as mensagens devem ser reenviadas.  
     * @return vetor com as mensagens a serem reenviadas.
     */
    vector<Ptr<MessageData>> searchForMsgsToResend(vector<Ptr<EventEARPWR>> events, Address a, int j);

    /** Verifica se todas as mensagens dos nós vizinhos foram reenviadas. Utilizado durante um rollback. */
    bool checkIfAllMsgsWereResent();

    /** 
     * Reprocessa as mensagens que foram reenviadas e conclui o procedimento de rollback. A aplicação
     * pode voltar ao processamento normal.
     */
    void reprocessMessagesAndConcludeRollback();

    /** 
     * Reprocessa as mensagens de REQUEST que foram reenviadas.
     */
    void reprocessRequestMessage(Ptr<MessageData> md);

    /** 
     * Reprocessa as mensagens de RESPONSE que foram reenviadas.
     */
    void reprocessResponseMessage(Ptr<MessageData> md);

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
    EARPWithoutRollback(Time timeInterval, int totalNodesQuantity, Ptr<CheckpointApp> application);

    /** Construtor padrão. */
    EARPWithoutRollback();

    ~EARPWithoutRollback() override;

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
    Ptr<EventEARPWR> getActiveEvent();

    //Especifica como deve ser feita a conversão desta classe em JSON
    virtual json to_json() override;

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    virtual void from_json(const json& j) override;

};

} // namespace ns3

#endif /* EARP_WITHOUT_ROLLBACK_H */
