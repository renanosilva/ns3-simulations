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

#include <ns3/string.h>
#include "ns3/application.h"
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

namespace ns3
{

/**
 * \ingroup checkpoint
 * \brief Classe que auxilia no processo de gerenciamento de checkpoints.
 */
class CheckpointHelper
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

    /** Remove todos os checkpoints existentes. */
    void removeAllCheckpointsAndLogs();

    /** Cria um novo checkpoint */
    void writeCheckpoint(string data);

    /** Cria um novo checkpoint, transformando um objeto em JSON. */
    void writeCheckpoint(Application *app);

    /** Cria um novo log */
    void writeLog(string data);

    /** Lê o conteúdo do último checkpoint criado. */
    string readLastCheckpoint();

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
    string readCheckpoint(int index);

    /** Especifica como um objeto da classe Application deve ser convertido em JSON (para checkpointing) */
    json to_json(const Application *app);

    /** Especifica como um JSON pode ser convertido em objeto da classe Application. */
    void from_json(const json& j, Application& app);

  private:

    string checkpointBaseName;       //nome de base do arquivo referente ao checkpoint
    int counter = 0;                 //contador que identifica quantos checkpoints já foram criados

    /** Remove todos os checkpoints existentes. */
    bool cleanDirectory(const std::string &path);

    /** Obtém o conteúdo de um arquivo. */
    string getFileContent(string filename);

    /** Escreve em um arquivo */
    void writeFile(string filename, nlohmann::json j);
};

} // namespace ns3

#endif /* BULK_SEND_HELPER_H */
