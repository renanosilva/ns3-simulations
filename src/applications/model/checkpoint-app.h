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
  private:

    virtual void StartApplication();
    virtual void StopApplication();

  protected:

    /** Define a estratégia de checkpointing a ser utilizada por este nó. */
    virtual void configureCheckpointStrategy();

    /////////////////////////////////////////////////////////////////////////////
    //////          ATRIBUTOS    NÃO    ARMAZENADOS EM CHECKPOINTS         //////
    /////////////////////////////////////////////////////////////////////////////

    /* 
      Por questão de organização, aqui devem ser declarados os atributos que não devem ser armazenados em 
      checkpoints. Exemplos desses tipos de atributos incluem atributos físicos, como, a carga atual da bateria, 
      ou atributos fixos (que nunca mudam) de uma aplicação.
    */

    /** Estratégia de checkpoint escolhida para este nó. */
    Ptr<CheckpointStrategy> checkpointStrategy;

    /** Helper que auxilia no gerenciamento das configurações do nó. */
    Ptr<ConfigHelper> configHelper;

    //Endereço IP do nó que iniciou o último procedimento de rollback
    Ipv4Address rollbackStarterIp;

    //ID do checkpoint para o qual deve ser feito rollback
    int checkpointId;

    /** Indica quais nós dependentes ainda precisam fazer rollback. */
    vector<Address> pendingRollbackAddresses;

    /** Indica quais nós dependentes ainda precisam confirmar criação de checkpoint. */
    vector<Address> pendingCheckpointAddresses;

    /**
     * Indica se um procedimento de rollback está em progresso. Quando um rollback
     * é iniciado, é necessário aguardar que todos os nós envolvidos o conclua para
     * que a comunicação possa ser restabelecida.
     */
    bool rollbackInProgress;

    /**
     * Indica se um procedimento de criação de checkpoints está em progresso. Quando 
     * um checkpoint é criado, é necessário aguardar que todos os nós envolvidos 
     * comuniquem sua conclusão para que o funcionamento normal da aplicação seja
     * retomado.
     */
    bool checkpointInProgress;

    //////////////////////////////////////////////////////////////
    //////       ATRIBUTOS ARMAZENADOS EM CHECKPOINTS       //////
    //////////////////////////////////////////////////////////////

    //Somente atributos de aplicação serão armazenados em checkpoints
    
    string nodeName;       //nome deste nó
    string configFilename; //nome do arquivo de configuração

    /** 
     * Endereços dos nós para os quais este nó enviou mensagens desde o último checkpoint.
     * Indica quais nós têm dependência com este nó, em caso de criação de checkpoints e 
     * de realização de rollbacks.
     */
    vector<Address> dependentAddresses;

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
     * (parcial, ainda não confirmado) para realizar algum processamento, caso seja necessário.
     * */
    virtual void beforePartialCheckpoint();

     /** 
     * Método abstrato. Método chamado imediatamente após a criação de um checkpoint
     * (parcial, ainda não confirmado) para realizar algum processamento, caso seja necessário.
     * */
    virtual void afterPartialCheckpoint();

    /** 
     * Método abstrato. Método chamado imediatamente antes do cancelamento de um checkpoint.
     * */
    virtual void beforeCheckpointDiscard();

    /** 
     * Método abstrato. Método chamado imediatamente após o cancelamento de um checkpoint.
     * */
    virtual void afterCheckpointDiscard();

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

    /**
     * Método abstrato. Serve para indicar em quais condições uma aplicação pode remover
     * um checkpoint ou não.
     * Por padrão, uma aplicação sempre pode remover checkpoints.
     * Caso não deseje dessa forma, a aplicação deve sobrescrever este método. 
     */
    virtual bool mayRemoveCheckpoint();

    /** 
     * Adiciona um endereço a um vetor de endereços. Não permite elementos repetidos.
     * @param v Vetor no qual o elemento será inserido.
     * @param a Endereço que será inserido no vetor.
     */
    void addAddress(vector<Address> &v, Address a);

    /** 
     * Remove um endereço de um vetor de endereços, mantendo a ordenação dos elementos. 
     * @param v Vetor do qual o elemento será removido.
     * @param a Endereço que será removido do vetor.
     * */
    void removeAddress(vector<Address> &v, Address a);

    /** Verifica se um determinado vetor de endereços possui um determinado elemento. */
    bool addressVectorContain(vector<Address> &v, const Address& a) {
      return find(v.begin(), v.end(), a) != v.end();
    }
    
};

} // namespace ns3

#endif /* CHECKPOINT_APP_H */
