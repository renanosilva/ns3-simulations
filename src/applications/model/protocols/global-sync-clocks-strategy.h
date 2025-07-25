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

#ifndef SYNC_PREDEFINED_TIMES_CHECKPOINT_H
#define SYNC_PREDEFINED_TIMES_CHECKPOINT_H

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

namespace ns3
{

/** 
 * Estratégia de checkpointing síncrona, na qual são gerados checkpoints 
 * a cada período predefinido de tempo. Todos os nós possuem relógios
 * sincronizados e criam checkpoints no mesmo instante de tempo.
 */
class GlobalSyncClocksStrategy : public CheckpointStrategy {

  private:

    /** Indica quais nós dependentes ainda precisam fazer rollback. */
    vector<Address> pendingRollbackAddresses;

    /** Indica quais nós dependentes ainda precisam confirmar criação de checkpoint. */
    vector<Address> pendingCheckpointAddresses;

    /** Intervalo de tempo no qual serão criados checkpoints. */
    Time interval;

    /** 
     * Timeout referente à criação do checkpoint. Se esse tempo for atingido e não houver
     * confirmação dos outros nós com relação à criação de seus checkpoints, o checkpoint
     * será excluído.
     * */
    Time timeout;

    /** Evento utilizado para criação de checkpoint. */
    EventId creationScheduling;

    /** Evento utilizado para cancelamento do último checkpoint criado. */
    EventId discardScheduling;

    /** Calcula a quantidade de segundos restantes até o próximo checkpoint. */
    Time getDelayToNextCheckpoint();

    /** Agenda a criação do próximo checkpoint. */
    void scheduleNextCheckpoint();

    /** Agenda a exclusão do último checkpoint. */
    void scheduleLastCheckpointDiscard();

    /** Confirma a criação do último checkpoint. */
    void confirmLastCheckpoint();

    /**
     * Intercepta a recepção de um pacote de um servidor enquanto ele está no modo de rollback.
     *
     * \param md Dados da mensagem recebida.
     * \return Retorna true caso a mensagem seja interceptada e processada pela estratégia de checkpoint. 
     * Nesse caso, a aplicação não deve processá-la. Retorna false caso contrário, ou seja, se a mensagem
     * não tiver sido processada, o que deverá ser feito pela aplicação.
     */
    bool interceptServerReadInRollbackMode(Ptr<MessageData> md);

    /**
     * Intercepta a recepção de um pacote de um servidor enquanto ele está no modo de criação de checkpoint.
     *
     * \param md Dados da mensagem recebida.
     * \return Retorna true caso a mensagem seja interceptada e processada pela estratégia de checkpoint. 
     * Nesse caso, a aplicação não deve processá-la. Retorna false caso contrário, ou seja, se a mensagem
     * não tiver sido processada, o que deverá ser feito pela aplicação.
     */
    bool interceptServerReadInCheckpointMode(Ptr<MessageData> md);

    /**
     * Intercepta a recepção de um pacote de um cliente enquanto ele está no modo de rollback.
     *
     * \param md Dados da mensagem recebida.
     * \return Retorna true caso a mensagem seja interceptada e processada pela estratégia de checkpoint. 
     * Nesse caso, a aplicação não deve processá-la. Retorna false caso contrário, ou seja, se a mensagem
     * não tiver sido processada, o que deverá ser feito pela aplicação.
     */
    bool interceptClientReadInRollbackMode(Ptr<MessageData> md);

    /**
     * Intercepta a recepção de um pacote de um cliente enquanto ele está no modo de criação de checkpoint.
     *
     * \param md Dados da mensagem recebida.
     * \return Retorna true caso a mensagem seja interceptada e processada pela estratégia de checkpoint. 
     * Nesse caso, a aplicação não deve processá-la. Retorna false caso contrário, ou seja, se a mensagem
     * não tiver sido processada, o que deverá ser feito pela aplicação.
     */
    bool interceptClientReadInCheckpointMode(Ptr<MessageData> md);

  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /** 
     * Construtor. 
     * @param interval Define a periodicidade (em segundos) na qual serão criados checkpoints.
     * @param timeout Timeout referente à criação do checkpoint. Se esse tempo for atingido e não houver
     * confirmação dos outros nós com relação à criação de seus checkpoints, o checkpoint
     * será excluído.
     * @param nodeName Nome do nó. Utilizado para criação de checkpoints.
     * @param application Aplicação que está sendo executada no nó. Os dados dessa classe serão armazenados em checkpoint.
     * 
     * */
    GlobalSyncClocksStrategy(Time timeInterval, Time timeout, Ptr<CheckpointApp> application);

    /** Construtor padrão. */
    GlobalSyncClocksStrategy();

    ~GlobalSyncClocksStrategy() override;

    virtual void startCheckpointing() override;
    
    virtual void stopCheckpointing() override;

    virtual void writeCheckpoint() override; 

    virtual void discardLastCheckpoint() override; 
    
    virtual void rollbackToLastCheckpoint() override; 

    virtual bool rollback(int checkpointId) override;

    /** 
     * Utilizado para iniciar um processo de rollback, após a solicitação
     * de um outro nó. O rollback será feito para o checkpoint identificado como
     * parâmetro.
     * Método abstrato. A implementação irá depender da estratégia adotada. 
     * @param requester Nó que solicitou o rollback.
     * @param checkpointId ID do checkpoint para o qual deverá ser feito rollback.
     * */
    virtual void rollback(Address requester, int cpId, string piggyBackedInfo = "") override;

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
     * Notifica os nós com os quais houve comunicação sobre a conclusão do checkpoint deste nó.
     */
    void notifyNodesAboutCheckpointCreated();

    /** 
     * Avisa ao iniciador de um rollback que este nó e seus dependentes concluíram seus procedimentos de rollback.
     * Marca o rollback como concluído, saindo do modo de bloqueio de comunicação.
     * */
    void concludeRollback();
    
    /** 
     * Conclui a criação de um checkpoint, após receber notificação dos outros nós dependentes,
     * ou o cancela. 
     * @param confirm Indica se o checkpoint será confirmado ou descartado.
    */
    virtual void confirmCheckpointCreation(bool confirm) override;

    /** 
     * Notifica os nós com os quais houve comunicação sobre a necessidade de realizarem rollback.
     * Método chamado quando este nó realiza seu próprio rollback.
     */
    void notifyNodesAboutRollback();

    //Especifica como deve ser feita a conversão desta classe em JSON
    virtual json to_json() override;

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    virtual void from_json(const json& j) override;
};

} // namespace ns3

#endif /* UDP_ECHO_SERVER_H */
