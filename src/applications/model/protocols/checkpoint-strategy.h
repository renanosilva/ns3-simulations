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
#include "ns3/message-data.h"
#include <string>

using namespace std;

namespace ns3
{

class CheckpointApp;
class CheckpointHelper;

/** Classe base para as classes que implementam estratégias de checkpointing. */
class CheckpointStrategy : public Object
{
  protected:

    /////////////////////////////////////////////////////////////////////////////
    //////          ATRIBUTOS    NÃO    ARMAZENADOS EM CHECKPOINTS         //////
    /////////////////////////////////////////////////////////////////////////////

    /* 
      Por questão de organização, aqui devem ser declarados os atributos que não devem ser armazenados em 
      checkpoints. Exemplos desses tipos de atributos incluem atributos físicos, como, a carga atual da bateria, 
      ou atributos fixos (que nunca mudam) de uma aplicação.
    */

    /** Aplicação na qual deverá ser feito o checkpoint. */
    Ptr<CheckpointApp> app;

    /** Auxilia a manipular os arquivos de checkpoint e logs. */
    Ptr<CheckpointHelper> checkpointHelper;

    /** 
     * Dados a serem armazenados em log, de forma a complementar os dados do 
     * checkpoint, se necessário. 
     * */
    string logData = "";

    /** Endereço IP do nó que iniciou o último procedimento de rollback */
    Address rollbackStarter;

    /** ID do checkpoint para o qual deve ser feito rollback */
    int checkpointId;

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

    /** 
     * Endereços dos nós para os quais este nó enviou mensagens desde o último checkpoint.
     * Indica quais nós têm dependência com este nó, em caso de criação de checkpoints e 
     * de realização de rollbacks.
     */
    vector<Address> dependentAddresses;

    /** 
     * Adiciona um endereço a um vetor de endereços. Não permite elementos repetidos.
     * @param v Vetor no qual o elemento será inserido.
     * @param a Endereço que será inserido no vetor.
     */
    void addAddress(vector<Address> &v, Address a);

     /** 
     * Adiciona um endereço ao vetor de endereços dependentes. Não permite elementos repetidos.
     * @param a Endereço que será inserido no vetor.
     */
    void addDependentAddress(Address a);

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

    /** Confirma a criação do último checkpoint. */
    virtual void confirmLastCheckpoint();

     /** 
     * Conclui a criação de um checkpoint, após receber notificação dos outros nós dependentes,
     * ou o cancela. 
     * @param confirm Indica se o checkpoint será confirmado ou descartado.
    */
    virtual void confirmCheckpointCreation(bool confirm);

    /** Método abstrato. Utilizado para descarte do último checkpoint criado. */
    virtual void discardLastCheckpoint();

    /** Obtém o identificador do último checkpoint criado. */
    int getLastCheckpointId();

  public:

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    CheckpointStrategy();
    
    ~CheckpointStrategy() override;

    /** Libera as referências armazenadas na classe. Idealmente, deve ser sobrescrito pela classe especializada. */
    virtual void DisposeReferences();

    /** 
     * Método abstrato. Utilizado para criação de checkpoints de fato. 
     * Serão armazenados no checkpoint os estados dos objetos de interesse.
     * A implementação irá depender da estratégia adotada.
     * */
    virtual void writeCheckpoint();

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
     * Método abstrato. Utilizado para criação de logs. 
     * Logs são uma forma de complemento aos checkpoints. Logs podem armazenar, por exemplo,
     * eventos ocorridos, como mensagens enviadas/recebidas, enquanto checkpoints armazenam
     * estados dos objetos. Logs podem armazenar eventos que precisam passar por replay após 
     * um rollback.
     * A implementação irá depender da estratégia adotada.
     * */
    virtual void writeLog();

    /** 
     * Método abstrato. Utilizado para iniciar um processo de rollback para o último
     * checkpoint criado, após a recuperação de um nó. A implementação irá depender 
     * da estratégia adotada. 
     * */
    virtual void rollbackToLastCheckpoint();

    /** 
     * Utilizado para iniciar um processo de rollback, após a recuperação
     * do próprio nó. O rollback é feito para o checkpoint identificado como
     * parâmetro.
     * Método abstrato. A implementação irá depender da estratégia adotada.
     * @param checkpointId ID do checkpoint para o qual deverá ser feito rollback.
     * @return Retorna se foi possível realizar o rollback ou não.
     * */
    virtual bool rollback(int checkpointId);

    /** 
     * Utilizado para iniciar um processo de rollback, após a solicitação
     * de um outro nó. O requisitor ficará armazenado para posterior comunicação
     * de conclusão. 
     * O rollback será feito para o checkpoint identificado como
     * parâmetro.
     * Método abstrato. A implementação irá depender da estratégia adotada. 
     * @param requester Nó que solicitou o rollback.
     * @param checkpointId ID do checkpoint para o qual deverá ser feito rollback.
     * */
    virtual void rollback(Address requester, int checkpointId, string piggyBackedInfo = "");

    /**
     * Intercepta a leitura de um pacote. Dessa forma, a estratégia de checkpoint
     * tem a oportunidade de processar a leitura antes da aplicação.
     *
     * \param md Dados da mensagem recebida.
     * \return Retorna true caso a mensagem seja interceptada e processada pela estratégia de checkpoint. 
     * Nesse caso, a aplicação não deve processá-la. Retorna false caso contrário, ou seja, se a mensagem
     * não tiver sido processada, o que deverá ser feito pela aplicação.
     */
    virtual bool interceptRead(Ptr<MessageData> md);

    /**
     * Método chamado após o recebimento de uma mensagem pela aplicação.
     *
     * \param md Dados da mensagem recebida.
     */
    virtual void afterMessageReceive(Ptr<MessageData> md);

    /**
     * Intercepta o envio de um pacote. Dessa forma, a estratégia de checkpoint
     * tem a oportunidade de analisar pacotes enviados pela aplicação.
     *
     * \param md Dados da mensagem enviada.
     * \return Retorna true caso a mensagem seja interceptada e processada pela estratégia de checkpoint. 
     * Nesse caso, a aplicação não deve processá-la. Retorna false caso contrário, ou seja, se a mensagem
     * não tiver sido processada, o que deverá ser feito pela aplicação.
     */
    virtual bool interceptSend(Ptr<MessageData> md);

    /** Indica se existe um procedimento de criação de checkpoint em progresso. */
    bool isCheckpointInProgress();

    /** Indica se existe um procedimento de rollback em progresso. */
    bool isRollbackInProgress();

    /** Especifica os dados a serem armazenados em log. */
    void setLogData(string data);

    /** Adiciona novos dados aos previamente existentes para serem armazenados em log. */
    void addLogData(string data);

    /** Obtém os dados a serem armazenados no próximo log. */
    string getLogData();

    /** 
     * Retorna o vetor de endereços que possuem dependência com este nó.
     * São endereços dos nós para os quais este nó enviou mensagens desde o último checkpoint.
     * Indica quais nós têm dependência com este nó, em caso de criação de checkpoints e 
     * de realização de rollbacks.
     */
    vector<Address> getDependentAddresses();

    virtual void printData();

    //Especifica como deve ser feita a conversão desta classe em JSON
    virtual json to_json();

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    virtual void from_json(const json& j);

};

} // namespace ns3

#endif
