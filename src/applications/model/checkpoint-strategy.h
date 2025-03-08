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

#ifndef CHECKPOINT_STRATEGY_H
#define CHECKPOINT_STRATEGY_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/type-id.h"
#include <ns3/double.h>
#include "ns3/checkpoint-app.h"
#include "ns3/checkpoint-helper.h"
#include <string>

using namespace std;

namespace ns3
{

class CheckpointApp;
class CheckpointHelper;

/** Classe base para as classes que implementam estratégias de checkpointing. */
class CheckpointStrategy : public Object
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    CheckpointStrategy();
    ~CheckpointStrategy() override;

    /** 
     * Método abstrato. Utilizado para realizar processamentos iniciais referente ao processo 
     * de criação de checkpoints. A implementação irá depender da estratégia adotada. 
     * */
    virtual void startCheckpointing();

    /** 
     * Método abstrato. Utilizado para parar o processo de criação de checkpoints de forma segura. 
     * A implementação irá depender da estratégia adotada. 
     * */
    virtual void stopCheckpointing();

    /** 
     * Método abstrato. Utilizado para criação de checkpoints de fato. 
     * Serão armazenados no checkpoint os estados dos objetos de interesse.
     * A implementação irá depender da estratégia adotada.
     * */
    virtual void writeCheckpoint();

    /** 
     * Método abstrato. Utilizado para criação de logs. 
     * Logs são uma forma de complemento aos checkpoints. Logs podem armazenar, por exemplo,
     * eventos ocorridos, como mensagens enviadas/recebidas, enquanto checkpoints armazenam
     * estados dos objetos. Logs podem armazenar eventos que precisam passar por replay após 
     * um rollback.
     * A implementação irá depender da estratégia adotada.
     * */
    virtual void writeLog();

    /** 
     * Método abstrato. Utilizado para iniciar um processo de rollback, após a recuperação
     * de um nó. A implementação irá depender da estratégia adotada. 
     * */
    virtual void startRollbackToLastCheckpoint();

    /** 
     * Utilizado para iniciar um processo de rollback, após a recuperação
     * de um nó. O nó faz o rollback para o checkpoint identificado como
     * parâmetro.
     * Método abstrato. A implementação irá depender da estratégia adotada. 
     * */
    virtual void startRollback(int checkpointId);

    /** Obtém o identificador do último checkpoint criado. */
    int getLastCheckpointId();

    /** Especifica os dados a serem armazenados em log. */
    void setLogData(string data);

    /** Adiciona novos dados aos previamente existentes para serem armazenados em log. */
    void addLogData(string data);

    /** Obtém os dados a serem armazenados no próximo log. */
    string getLogData();

    void setApp(Ptr<CheckpointApp> application);

    //Especifica como deve ser feita a conversão desta classe em JSON
    friend void to_json(json& j, const CheckpointStrategy& obj);

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    friend void from_json(const json& j, CheckpointStrategy& obj);

    //friend class BatteryNodeAp; 

  protected:

    /** Auxilia a manipular os arquivos de checkpoint e logs. */
    Ptr<CheckpointHelper> checkpointHelper;

    /** Dados a serem armazenados no checkpoint. */
    string logData = "";

    /** Aplicação na qual deverá ser feito o checkpoint. */
    Ptr<CheckpointApp> app;

};

} // namespace ns3

#endif
