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

#include "ns3/application.h"
#include "ns3/checkpoint-strategy.h"
#include "ns3/ptr.h"
#include "ns3/config-helper.h"
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>

using json = nlohmann::json;
using namespace std;

namespace ns3
{

class CheckpointStrategy; 

/**
 * \ingroup applications
 * \defgroup applications
 */

/**
 * \ingroup sensor
 * \brief Aplicação referente a um nó que realiza checkpoint.
 */
class CheckpointApp : public Application
{
  public:

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    CheckpointApp();
    ~CheckpointApp() override;

    string getNodeName();

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

    /** 
     * Método abstrato. Método chamado imediatamente antes da criação de um checkpoint
     * para realizar algum processamento, caso seja necessário.
     * */
    virtual void beforeCheckpoint();

    /** 
     * Método abstrato. Método chamado imediatamente após a criação de um checkpoint
     * para realizar algum processamento, caso seja necessário.
     * */
    virtual void afterCheckpoint();

    /** 
     * Método abstrato. Método chamado imediatamente antes da execução de um rollback
     * para realizar algum processamento, caso seja necessário.
     * */
    virtual void beforeRollback();

    /** 
     * Método abstrato. Método chamado imediatamente após a execução de um rollback
     * para realizar algum processamento, caso seja necessário.
     * */
    virtual void afterRollback();

    /**
     * Método abstrato. Serve para indicar em quais condições uma aplicação pode criar
     * um checkpoint ou não.
     * Por padrão, uma aplicação sempre pode criar checkpoints.
     * Caso não deseje dessa forma, a aplicação deve sobrescrever este método. 
     */
    virtual bool mayCheckpoint();

    //friend class CheckpointStrategy;

  private:

    virtual void StartApplication();
    virtual void StopApplication();

  protected:

    /** Define a estratégia de checkpointing a ser utilizada por este nó. */
    virtual void configureCheckpointStrategy();

    ////////////////////////////////////////////////
    //////          ATRIBUTOS NATIVOS         //////
    ////////////////////////////////////////////////

    /* 
      Atributos nativos não são controlados pela aplicação. Podem ser atributos físicos,
      como, por exemplo, a carga atual da bateria, ou atributos fixos (que nunca mudam) 
      de uma aplicação. Esses atributos não são incluídos em checkpoints.
    */

    /** Estratégia de checkpoint escolhida para este nó. */
    Ptr<CheckpointStrategy> checkpointStrategy;

    /** Helper que auxilia no gerenciamento das configurações do nó. */
    Ptr<ConfigHelper> configHelper;

    ////////////////////////////////////////////////
    //////       ATRIBUTOS DE APLICAÇÃO       //////
    ////////////////////////////////////////////////

    //Somente atributos de aplicação serão armazenados em checkpoints
    string nodeName;       //nome deste nó
    string configFilename; //nome do arquivo de configuração
    
};

} // namespace ns3

#endif /* CHECKPOINT_APP_H */
