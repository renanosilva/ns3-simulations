/*
 * Copyright (c) 2008 INRIA
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
 *
 * Author: Geoge Riley <riley@ece.gatech.edu>
 * Adapted from OnOffHelper by:
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef CHECKPOINT_HELPER_H
#define CHECKPOINT_HELPER_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include <ns3/string.h>
#include "ns3/checkpoint-app.h"
#include <nlohmann/json.hpp>
#include "ns3/names.h"
#include "ns3/uinteger.h"

using namespace std;
using json = nlohmann::json;

namespace ns3
{

class CheckpointApp;

/**
 * \ingroup checkpoint
 * \brief Classe que auxilia no processo de gerenciamento de checkpoints.
 */
class CheckpointHelper : public Object
{
  public:

    /** Construtor padrão. */
    CheckpointHelper();

    /**
     * Construtor.
     *
     * \param checkpointBaseName nome de base para os checkpoints a serem criados. O nome
     * serve para auxiliar a identificar a qual nó o checkpoint se refere.
     */
    CheckpointHelper(string checkpointBaseName);

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /** Remove todos os checkpoints existentes. */
    void removeAllCheckpointsAndLogs();

    /** Cria um novo checkpoint */
    void writeCheckpoint(string data, int checkpointId);

    /** 
     * Cria um novo checkpoint, transformando um objeto em JSON. 
     * @param app Aplicação do nó que irá criar o checkpoint
     * @param CheckpointId ID do checkpoint a ser criado
    */
    void writeCheckpoint(Ptr<CheckpointApp> app, int checkpointId);

    /** Lê o conteúdo do último checkpoint criado. */
    json readLastCheckpoint();

    /** 
     * Retorna o nome do arquivo do iésimo checkpoint.
     * @param i índice do checkpoint.
     */
    string getCheckpointFilename(int i);

    /** 
     * Retorna o nome do arquivo do iésimo log.
     * @param i índice do log.
     */
    string getLogFilename(int i);

    /** 
     * Retorna o nome de base dos arquivos de checkpoint, que representa o nome do nó.
     */
    string getCheckpointBasename();

    /** 
     * Retorna o nome de base dos arquivos de log de um nó.
     */
    string getLogBasename();

    /** 
     * Lê o conteúdo de um checkpoint. 
     * @param index índice do checkpoint a ser lido.
     * */
    json readCheckpoint(int index);

    /** 
     * Obtém o último índice de um checkpoint criado, lendo diretamente da base de arquivos, caso seja necessário. 
     * Caso tenha sido feito algum rollback, retorna o índice do último checkpoint a ser usado.
     * */
    int getLastCheckpointId();

    //Especifica como deve ser feita a conversão desta classe em JSON
    friend void to_json(json& j, const CheckpointHelper& obj);

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    friend void from_json(const json& j, CheckpointHelper& obj);

    friend class BatteryNodeApp;

  private:

    string checkpointBaseName;       //nome de base do arquivo referente ao checkpoint
    int lastCheckpointId = 0;        //contador que identifica o ID do último checkpoint criado

    /** Remove todos os checkpoints existentes. */
    bool cleanDirectory(const std::string &path);

    /** 
     * Dado um vetor de strings (com o padrão de nomenclatura dos arquivos de checkpoint), retorna
     * o maior ID de checkpoint.
     */
    int findMaxIdFromFilenames(const vector<std::string>& filenames);

    /** Obtém o conteúdo de um arquivo. */
    string getFileContent(string filename);

    /** Lista os arquivos de uma determinada pasta, de acordo com um padrão de busca. */
    vector<string> listFiles(const string& pasta, const string& padrao);

    /** Escreve em um arquivo */
    void writeFile(string filename, nlohmann::json j);
};

} // namespace ns3

#endif /* BULK_SEND_HELPER_H */
