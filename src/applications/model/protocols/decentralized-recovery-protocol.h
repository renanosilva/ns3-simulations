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

#ifndef DECENTRALIZED_RECOVERY_PROTOCOL_H
#define DECENTRALIZED_RECOVERY_PROTOCOL_H

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
 * Decentralized Recovery Protocolo Node Info.
 * Representa informações que precisam ser armazenadas sobre outros nós
 * para que o protocolo funcione corretamente. 
*/
class DRPNodeInfo {
  
  private:
    
    Address address; //endereço do nó
    int activeCheckpoint; //ID do último checkpoint do nó em questão

  public:

    DRPNodeInfo(){};
    
    DRPNodeInfo(const Address& addr, int ac)
      : address(addr), activeCheckpoint(ac) {}

    Address GetAddress() const { return address; }
    int GetActiveCheckpoint() const { return activeCheckpoint; }

    void SetAddress(const Address& a) { address = a; }
    void SetActiveCheckpoint(const int& c) { activeCheckpoint = c; }

    //Especifica como deve ser feita a conversão desta classe em JSON
    friend void to_json(json& j, const DRPNodeInfo& obj);

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    friend void from_json(const json& j, DRPNodeInfo& obj);
};

/** 
 * Estratégia de checkpointing assíncrona, na qual são gerados checkpoints 
 * a cada determinado período de tempo, que podem ser diferentes entre cada nó. 
 */
class DecentralizedRecoveryProtocol : public CheckpointStrategy {

  private:

    //////////////////////////////////////////////////////////////
    //////       ATRIBUTOS ARMAZENADOS EM CHECKPOINTS       //////
    //////////////////////////////////////////////////////////////

    /** Checkpoint sequence number. */
    int csn;

    /** 
     * Utilizado para armazenar informações sobre outros nós com os quais este nó
     * possua dependência, como seus endereços e ID de checkpoint ativo (último 
     * checkpoint criado).
     */
    vector<DRPNodeInfo> propList;

    /////////////////////////////////////////////////////////////////////////////
    //////          ATRIBUTOS    NÃO    ARMAZENADOS EM CHECKPOINTS         //////
    /////////////////////////////////////////////////////////////////////////////

    /** Checkpoint ativo do nó. */
    int activeCheckpoint;

    /** Intervalo de tempo no qual serão criados checkpoints. */
    Time interval;

    /** Evento utilizado para criação de checkpoint. */
    EventId creationScheduling;

    /** Calcula a quantidade de segundos restantes até o próximo checkpoint. */
    Time getDelayToNextCheckpoint();

    /** Agenda a criação do próximo checkpoint. */
    void scheduleNextCheckpoint();

    /** 
     * Adiciona um novo elemento à propList, caso ele já não exista com a mesma informação
     * de checkpoint ativo. Não adiciona caso o checkpoint que se está tentando adicionar
     * seja posterior ao que já está presente. Isso é feito pois, caso seja necessário um
     * rollback, ele deverá ser feito para o checkpoint mais antigo.
     * 
     * @param i Informações que se está tentando inserir na propList.
     */
    void addToPropList(DRPNodeInfo i);

    /** Atualiza a propList do último checkpoint para a propList atual do nó. */
    void editActiveCheckpointPropList();

    /** 
     * Busca um objeto DRPNodeInfo na propList que possua um determinado Address. 
     * @param addr Endereço que se está buscando na propList.
    */
    DRPNodeInfo* findNodeInfoByAddress(const Address& addr);

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
    DecentralizedRecoveryProtocol(Time timeInterval, Ptr<CheckpointApp> application);

    /** Construtor padrão. */
    DecentralizedRecoveryProtocol();

    ~DecentralizedRecoveryProtocol() override;

    virtual void startCheckpointing() override;
    
    virtual void stopCheckpointing() override;

    virtual void writeCheckpoint() override; 

    /** 
     * Método utilizado para realizar um rollback para o último checkpoint válido após a recuperação
     * de uma falha do próprio nó. Notifica nós dependentes.
     */
    virtual void rollbackToLastCheckpoint() override; 

    /** 
     * Utilizado para iniciar um processo de rollback, após a solicitação
     * de um outro nó. O rollback será feito para o checkpoint identificado como
     * parâmetro.
     * Método abstrato. A implementação irá depender da estratégia adotada. 
     * @param requester Nó que solicitou o rollback.
     * @param checkpointId ID do checkpoint para o qual deverá ser feito rollback.
     * */
    virtual void rollback(Address requester, int cpId, string piggyBackedInfo = "") override;

    /** Realiza um rollback para um checkpoint específico. */
    virtual bool rollback(int checkpointId) override;

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
     * Método chamado quando este nó realiza seu próprio rollback.
     */
    void notifyAllNodesAboutRollback();

    /** 
     * Notifica outros nós sobre a necessidade de realizarem rollback.
     * Não notifica o solicitante de um rollback, caso ele já tenha feito
     * rollback para o checkpoint consistente.
     */
    void notifyNodesAboutRollback(Address requester, int requesterActiveCheckpoint);

    virtual void printData() override;

    //Especifica como deve ser feita a conversão desta classe em JSON
    virtual json to_json() override;

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    virtual void from_json(const json& j) override;
};

} // namespace ns3

#endif /* DECENTRALIZED_RECOVERY_PROTOCOL */
