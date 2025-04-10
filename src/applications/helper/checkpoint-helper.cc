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

#include "checkpoint-helper.h"
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <vector>
#include "ns3/simulator.h"
#include "ns3/battery-node-app.h"
#include <regex>
#include <cstdio>

using namespace std;

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CheckpointHelper");
NS_OBJECT_ENSURE_REGISTERED(CheckpointHelper);

/** Pasta onde se encontram os checkpoints dos nós. */
static const string CHECKPOINTS_FOLDER = "checkpoints/";

CheckpointHelper::CheckpointHelper()
{
    this->checkpointBaseName = "";
}

CheckpointHelper::CheckpointHelper(string baseName)
{
    this->checkpointBaseName = baseName;
}

TypeId CheckpointHelper::GetTypeId() {
    NS_LOG_FUNCTION("CheckpointHelper::GetTypeId()");

    static TypeId tid = TypeId("ns3::CheckpointHelper")
        .SetParent<Object>()
        .SetGroupName("Checkpoint")
        .AddConstructor<CheckpointHelper>()
        .AddAttribute("CheckpointBaseName", "Nome base para os checkpoints criados.",
                      StringValue(""),
                      MakeStringAccessor(&CheckpointHelper::checkpointBaseName),
                      MakeStringChecker())
        .AddAttribute("LastCheckpointId", "Contador que identifica o ID do último checkpoint criado.",
                      UintegerValue(0),  // Alterado para UintegerValue
                      MakeUintegerAccessor(&CheckpointHelper::lastCheckpointId),
                      MakeUintegerChecker<int>());
    
    NS_LOG_FUNCTION("Fim do método");

    return tid;
}

string CheckpointHelper::getCheckpointFilename(int i){
    NS_LOG_FUNCTION(this);
    return CHECKPOINTS_FOLDER + checkpointBaseName + "-" + to_string(i) + ".json";
}

string CheckpointHelper::getLogFilename(int i){
    NS_LOG_FUNCTION(this);
    return CHECKPOINTS_FOLDER + getLogBasename() + "-" + to_string(i) + ".json";
}

void CheckpointHelper::removeAllCheckpointsAndLogs()
{
    NS_LOG_FUNCTION(this);
    cleanDirectory(CHECKPOINTS_FOLDER);
    NS_LOG_FUNCTION("Fim do método");
}

bool CheckpointHelper::cleanDirectory(const std::string &path){
    NS_LOG_FUNCTION(this);
    
    struct dirent *ent;
    DIR *dir = opendir(path.c_str());
    if (dir != NULL) {
        /* remove all the files and directories within directory */
        while ((ent = readdir(dir)) != NULL) {
            std::remove((path + ent->d_name).c_str());
        }
        closedir (dir);
    } else {
        /* could not open directory */
        return false;
    }

    NS_LOG_FUNCTION("Fim do método");
    return true;
}

vector<string> CheckpointHelper::listFiles(const string& pasta, const string& padrao) {
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(!pasta.empty(), "Erro: Caminho do diretório vazio!");

    DIR* dir = opendir(pasta.c_str());

    NS_ASSERT_MSG(dir != nullptr, "Erro ao abrir o diretório!");

    struct dirent* entry;
    vector<string> arquivos;

    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG) { // Se for um arquivo regular
            if (!padrao.empty() && std::strstr(entry->d_name, padrao.c_str()) != nullptr) {
                arquivos.push_back(std::string(entry->d_name));  // Copia segura
            }
        }
    }

    closedir(dir);

    NS_LOG_FUNCTION("Fim do método");

    return arquivos;
}

void CheckpointHelper::writeCheckpoint(string data, int checkpointId){
    NS_LOG_FUNCTION(this);
    
    nlohmann::json j;
    j.push_back(data);

    writeFile(getCheckpointFilename(checkpointId), j);
    lastCheckpointId = checkpointId;
    
    NS_LOG_LOGIC("\n" << checkpointBaseName << " - CHECKPOINT CRIADO. ID: " << lastCheckpointId << "\n");
    NS_LOG_FUNCTION("Fim do método");
}

void CheckpointHelper::writeCheckpoint(Ptr<CheckpointApp> app, int checkpointId){
    NS_LOG_FUNCTION(this);

    //nlohmann::json j = to_json(app);
    json j = app->to_json();

    writeFile(getCheckpointFilename(checkpointId), j);
    lastCheckpointId = checkpointId;
    
    NS_LOG_LOGIC("\n" << checkpointBaseName << " - CHECKPOINT CRIADO. ID: " << lastCheckpointId << "\n");
    NS_LOG_FUNCTION("Fim do método");
}

void CheckpointHelper::removeCheckpoint(int checkpointId){
    NS_LOG_FUNCTION(this);

    string filename = getCheckpointFilename(checkpointId);
    std::remove(filename.c_str());
    
    NS_LOG_FUNCTION("Fim do método");
}  

void CheckpointHelper::writeFile(string filename, nlohmann::json j){
    NS_LOG_FUNCTION(this);

    string content = getFileContent(filename);

    // Create and open a text file
    ofstream logFile(filename);

    logFile << content << j;

    logFile.close();

    NS_LOG_FUNCTION("Fim do método");
}

string CheckpointHelper::getFileContent(string filename)
{
    NS_LOG_FUNCTION(this);

    ifstream t(filename);
    stringstream buffer;
    buffer << t.rdbuf();

    NS_LOG_FUNCTION("Fim do método");
    return buffer.str();
}

json CheckpointHelper::readCheckpoint(int index)
{   
    NS_LOG_FUNCTION(this);
    string c = getFileContent(getCheckpointFilename(index));
    NS_LOG_FUNCTION("Fim do método");
    return json::parse(c);
}

json CheckpointHelper::readLastCheckpoint()
{
    NS_LOG_FUNCTION(this);

    string content = getFileContent(getCheckpointFilename(getLastCheckpointId()));

    /*if (content.length() == 0){
        content = getFileContent(getCheckpointFilename(getLastCheckpointId()-1));
    }*/

    NS_LOG_FUNCTION("Fim do método");
    return json::parse(content);
}

string CheckpointHelper::getCheckpointBasename(){
    NS_LOG_FUNCTION(this);
    return checkpointBaseName;
}

string CheckpointHelper::getLogBasename(){
    NS_LOG_FUNCTION(this);
    return checkpointBaseName + "-log";
}

int CheckpointHelper::getLastCheckpointId(){
    NS_LOG_FUNCTION(this);

    if (lastCheckpointId != 0){
        return lastCheckpointId;
    }

    //obtendo o último checkpoint criado a partir dos nomes dos arquivos dos checkpoints
    vector<string> nomes = listFiles(CHECKPOINTS_FOLDER, checkpointBaseName);

    if (nomes.empty())
        return -1;

    lastCheckpointId = findMaxIdFromFilenames(nomes);

    NS_LOG_LOGIC("\n" << checkpointBaseName << " - CONTADOR DE CHECKPOINT RESTAURADO: " << lastCheckpointId << "\n");
    NS_LOG_FUNCTION("Fim do método");

    //retorna o valor do último checkpoint válido
    return lastCheckpointId;
    
}

int CheckpointHelper::findMaxIdFromFilenames(const vector<std::string>& filenames) {
    NS_LOG_FUNCTION(this);

    string maxStr;
    int maxNumber = -1;

    for (const auto& filename : filenames) {
        size_t lastDash = filename.rfind('-'); // Último '-'
        size_t dotJson = filename.rfind(".json"); // Posição de ".json"
        
        if (lastDash != std::string::npos && dotJson != std::string::npos) {
            int num = std::stoi(filename.substr(lastDash + 1, dotJson - lastDash - 1));
            if (num > maxNumber) {
                maxNumber = num;
                maxStr = filename;
            }
        }
    }

    NS_LOG_FUNCTION("Fim do método");

    return maxNumber;
    
}

void to_json(json& j, const CheckpointHelper& obj) {
    NS_LOG_FUNCTION("CheckpointHelper::to_json");

    j = json{
        {"checkpointBaseName", obj.checkpointBaseName}, 
        {"lastCheckpointId", obj.lastCheckpointId}
    };

    NS_LOG_FUNCTION("Fim do método");
}

void from_json(const json& j, CheckpointHelper& obj) {
    NS_LOG_FUNCTION("CheckpointHelper::from_json");

    j.at("checkpointBaseName").get_to(obj.checkpointBaseName);
    j.at("lastCheckpointId").get_to(obj.lastCheckpointId);

    NS_LOG_FUNCTION("Fim do método");
}

} // namespace ns3
