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

#ifndef ECS_PROTOCOL
#define ECS_PROTOCOL

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
 * Estrutura responsável por armazenar o contador de eventos de um nó.
 * A estrutura é utilizada para garantir que o valor inicial do contador
 * seja -1.
 */
struct EventCount {
    int value;
    EventCount() : value(-1) {}  // valor padrão -1
};

/** Armazena informações sobre uma mensagem enviada e ainda pendente de confirmação (ack). */
class MessageSenderData : public Object {
  
private:
  
  Ptr<MessageData> messageData; //Emissor da mensagem pendente de confirmação
  int sendEvent; //Evento do nó emissor durante o qual a mensagem foi enviada
  
public:

  MessageSenderData(){};

  ~MessageSenderData(){
    messageData = Create<MessageData>();
    sendEvent = -1;
  }
  
  int GetSendEvent() const { return sendEvent; }
  Ptr<MessageData> GetMessageData() const { return messageData; }

  void SetSendEvent(const int& j) { sendEvent = j; }
  void SetMessageData(const Ptr<MessageData>& s) { messageData = s; }

  //Especifica como deve ser feita a conversão desta classe em JSON
  friend void to_json(json& j, const Ptr<MessageSenderData>& obj);

  //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
  friend void from_json(const json& j, Ptr<MessageSenderData>& obj);
};

/** 
 * Representa um evento, que pode ser o envio ou recebimento de uma mensagem.
 * Um evento fica armazenado na memória volátil do nó até que, de tempos em tempos,
 * é gravado no armazenamento estável, o que representa a criação do checkpoint.
 * Um evento armazena informações cruciais para o rastreamento das mudanças
 * de estados pelos quais o nó passa.
 * 
*/
class EventRecord : public Object {
  
private:
  
  int j; //índice do evento
  Ptr<MessageData> m; //mensagem recebida no evento (pode ser nulo)
  vector<Ptr<MessageData>> mSent; //conjunto de mensagens enviadas durante o evento (pode ser nulo)
  
public:

  EventRecord(){};

  ~EventRecord(){
    m = nullptr;
    mSent.clear();
  }
  
  int GetJ() const { return j; }
  Ptr<MessageData> GetMessage() const { return m; }
  vector<Ptr<MessageData>> GetMSent() const { return mSent; }

  void SetJ(const int& a) { j = a; }
  void SetMessage(const Ptr<MessageData>& message) { m = message; }
  void SetMSent(const vector<Ptr<MessageData>>& messages) { mSent = messages; }

  //Especifica como deve ser feita a conversão desta classe em JSON
  friend void to_json(json& j, const Ptr<EventRecord>& obj);

  //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
  friend void from_json(const json& j, Ptr<EventRecord>& obj);
};

/** 
 * Protocolo de checkpointing e rollback utilizado na ECS.
 * Por ser bastante diferente dos demais nós, utilizou-se uma classe derivada.
 */
class ECSProtocol : public CheckpointStrategy {

  private:

    //////////////////////////////////////////////////////////////
    //////       ATRIBUTOS ARMAZENADOS EM CHECKPOINTS       //////
    //////////////////////////////////////////////////////////////

    /** 
     * Armazena os eventos ocorridos no nó (desde o último checkpoint).
     * Armazenado diretamente na memória estável.
     */
    // vector<Ptr<EventRecord>> events;

    /** 
     * Mapa de mensagens cujos recebimentos ainda não foram confirmados.
     * Chave: endereço do nó.
     * Valor: mensagens enviadas para o nó cujos recebimentos ainda não foram
     * confirmados.
     */
    map<Ipv4Address, vector<Ptr<MessageSenderData>>> pendingAck;

    /** 
     * Mapa contendo o último contador de eventos de cada nó.
     * 
     * Chave: endereço do nó.
     * Valor: contador de eventos do nó.
     */
    map<Ipv4Address, EventCount> eventCountMap;

    /////////////////////////////////////////////////////////////////////////////
    //////          ATRIBUTOS    NÃO    ARMAZENADOS EM CHECKPOINTS         //////
    /////////////////////////////////////////////////////////////////////////////

    /** 
     * Nós que estão em falha, de acordo com o que este nó sabe.
     * Utilizado para saber se, durante uma recuperação, uma
     * mensagem precisa ser reenviada para outro nó que falhou.
     * Chave: endereço do nó remoto que falhou.
     * Valor: quantidade de mensagens que o nó remoto alega ter recebido deste nó.
     */
    vector<Address> failedNodes;

    /** 
     * Retorna todos os eventos que ocorreram após um evento específico em determinado nó.
     *
     * @param node endereço do nó cujos eventos se está buscando.
     * @param refEventIndex índice de referência do evento buscado.  
     * @return vetor de eventos que ocorreram após o evento buscado.
     */
    json getEventsAfterIndex(Address node, int refEventIndex);
    
    /** 
     * Cria um checkpoint de um nó, após receber uma solicitação.
     * @param md Mensagem de requisição recebida com os dados do checkpoint a ser criado.
     */
    void afterCheckpointRequestReceived(Ptr<MessageData> md);

    /** 
     * Envia para o nó solicitante seu último checkpoint, assim como as mensagens que ele
     * precisa reexecutar.
     *
     * @param md Mensagem de requisição recebida com os dados do checkpoint a ser criado.
     */
    void afterRequestResendReceive(Ptr<MessageData> md);

    /** 
     * Verifica se uma mensagem possui acks embutidos e, caso tenha, os remove do vetor
     * de acks pendentes e faz o registro no log de eventos.
     * @param md Mensagem recebida pela ECS a ser analisada.
     */
    void checkForAcksAndLogEvents(Ptr<MessageData> md);

    /** Processa o envio de uma mensagem de requisição de um nó.
     * 
     * \param md Dados da mensagem recebida.
     * \return Retorna true caso a mensagem seja interceptada e processada pela estratégia de checkpoint. 
     * Nesse caso, a aplicação não deve mais processá-la. Retorna false caso contrário, ou seja, se a 
     * mensagem não tiver sido processada, o que deverá ser feito pela aplicação.
     * 
     */
    bool processRequestSendEvent(Ptr<MessageData> md);

    /** Processa o recebimento de uma mensagem de resposta de um nó.
     * 
     * \param md Dados da mensagem recebida.
     * \return Retorna true caso a mensagem seja interceptada e processada pela estratégia de checkpoint. 
     * Nesse caso, a aplicação não deve mais processá-la. Retorna false caso contrário, ou seja, se a 
     * mensagem não tiver sido processada, o que deverá ser feito pela aplicação.
     * 
     */
    // bool processResponseReceiveEvent(Ptr<MessageData> md);

    /** Intercepta o envio de uma mensagem de resposta de um nó.
     * 
     * \param md Dados da mensagem recebida.
     * \return Retorna true caso a mensagem seja interceptada e processada pela estratégia de checkpoint. 
     * Nesse caso, a aplicação não deve mais processá-la. Retorna false caso contrário, ou seja, se a 
     * mensagem não tiver sido processada, o que deverá ser feito pela aplicação.
     * 
     */
    bool processResponseSendEvent(Ptr<MessageData> md);

    /** Retorna o nome do arquivo do checkpoint de um nó. */
    string getCheckpointFileName(Address from);

    /** Retorna o nome do log de eventos de um nó. */
    string getEventLogFileName(Address from);

  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /** 
     * Construtor. 
     * @param totalNodesQuantity Quantidade total de nós do sistema.
     * @param application Aplicação da ECS.
     * 
     * */
    ECSProtocol(Ptr<CheckpointApp> application);

    /** Construtor padrão. */
    ECSProtocol();

    ~ECSProtocol() override;

    /** Remove as referências armazenadas na classe. */
    virtual void DisposeReferences() override;

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

};

} // namespace ns3

#endif /* ECSProtocol */
