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

#ifndef NO_ROLLBACK_2_WITH_CSS_H
#define NO_ROLLBACK_2_WITH_CSS_H

#include "ns3/ecs-protocol.h"
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
#include "ns3/ecs-checkpoint-app.h"
#include <ns3/double.h>
#include <unordered_set>

namespace ns3
{

/** 
 * Protocolo No Rollback 2 modificado para inclusão de uma estação central (CSS).
 * 
 * Estratégia de checkpointing assíncrona, na qual são gerados checkpoints 
 * a cada determinado período de tempo, que podem ser diferentes entre cada nó.
 * Nessa versão do protocolo, somente o nó
 * que falhou precisa fazer rollback, sem limite de nós que podem falhar no sistema.
 * O nó central é responsável por encaminhar mensagens a seus destinos finais e
 * por armazenar os checkpoints de todos os nós.
 */
class NoRollback2WithECS : public CheckpointStrategy {

  private:

    //////////////////////////////////////////////////////////////
    //////       ATRIBUTOS ARMAZENADOS EM CHECKPOINTS       //////
    //////////////////////////////////////////////////////////////

    /** Contador de eventos. */
    int eventCount;

    /////////////////////////////////////////////////////////////////////////////
    //////          ATRIBUTOS    NÃO    ARMAZENADOS EM CHECKPOINTS         //////
    /////////////////////////////////////////////////////////////////////////////

    /** Mensagens cujos recebimentos ainda precisam ser confirmados por este nó para a ECS. */
    vector<Ptr<MessageData>> pendingAcksToSend;

    /** Aplicação na qual deverá ser feito o checkpoint. */
    Ptr<ECSCheckpointApp> app;

    /** Intervalo de tempo no qual serão criados checkpoints. */
    Time interval;

    /** Evento utilizado para criação de checkpoint. */
    EventId creationScheduling;

    /** Calcula a quantidade de segundos restantes até o próximo checkpoint. */
    Time getDelayToNextCheckpoint();

    /** Agenda a criação do próximo checkpoint. */
    void scheduleNextCheckpoint();

    /** Reexecuta um determinado evento, após a restauração do estado da aplicação associado a ele. */
    void replayEvent(Ptr<EventRecord> e);

    /** Realiza um processamento após o recebimento de uma mensagem reenviada. */
    void afterResendMessageReceive(Ptr<MessageData> md);

    /** Adiciona acks pendentes de envio a uma mensagem, na forma de piggyback. */
    void addAcksToMessage(Ptr<MessageData> md);

    /**
     * Retorna valores de interesse referentes a uma mensagem reenviada.
     * 
     * @param piggyBackedData Payload enviado.
     * @param key Nome do parâmetro a ser obtido (checkpointData, events ou pendingAck).
     */
    string getResentData(const string& piggyBackedData, const string& key);

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
    NoRollback2WithECS(Time timeInterval, Ptr<CheckpointApp> application);

    /** Construtor padrão. */
    NoRollback2WithECS();

    ~NoRollback2WithECS() override;

    /** Remove as referências armazenadas na classe. */
    virtual void DisposeReferences() override;

    virtual void startCheckpointing() override;
    
    virtual void stopCheckpointing() override;

    void requestCheckpointCreation();

    /** 
     * Método utilizado para realizar um rollback para o último checkpoint válido após a recuperação
     * de uma falha do próprio nó.
     */
    virtual void rollbackToLastCheckpoint() override; 

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

    /** 
     * Dá a oportunidade de realizar algum processamento imediatamente antes da bateria do nó se 
     * descarregar. 
     * */
    virtual void beforeBatteryDischarge() override;

    virtual void printData() override;

    //Especifica como deve ser feita a conversão desta classe em JSON
    virtual json to_json() override;

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    virtual void from_json(const json& j) override;

};

} // namespace ns3

#endif /* NO_ROLLBACK_2_WITH_CSS_H */
