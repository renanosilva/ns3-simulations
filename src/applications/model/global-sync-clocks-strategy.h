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
    
    virtual void startRollbackToLastCheckpoint() override; 

    virtual void startRollback(int checkpointId) override;

    //Especifica como deve ser feita a conversão desta classe em JSON
    friend void to_json(json& j, const GlobalSyncClocksStrategy& obj);

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    friend void from_json(const json& j, GlobalSyncClocksStrategy& obj);
};

} // namespace ns3

#endif /* UDP_ECHO_SERVER_H */
