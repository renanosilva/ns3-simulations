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
#include <string>
#include "ns3/simulator.h"
#include "ns3/server-node-app.h"
#include <regex>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <algorithm>


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

    return arquivos;
}

void CheckpointHelper::writeCheckpoint(string data, int checkpointId){
    NS_LOG_FUNCTION(this);
    
    nlohmann::json j;
    j.push_back(data);

    writeFile(getCheckpointFilename(checkpointId), j);
    lastCheckpointId = checkpointId;
    
    NS_LOG_LOGIC("\n" << checkpointBaseName << " - CHECKPOINT CRIADO. ID: " << lastCheckpointId << "\n");
}

void CheckpointHelper::writeCheckpoint(Ptr<CheckpointApp> app, int checkpointId){
    NS_LOG_FUNCTION(this);

    json j = app->to_json();

    writeFile(getCheckpointFilename(checkpointId), j);
    lastCheckpointId = checkpointId;
    
    NS_LOG_LOGIC("\n" << checkpointBaseName << " - CHECKPOINT CRIADO. ID: " << lastCheckpointId << "\n");
}

void CheckpointHelper::writeCheckpoint(Ptr<CheckpointApp> app, int checkpointId, bool confirmed){
    NS_LOG_FUNCTION(this);

    json j = app->to_json();
    j["confirmed"] = confirmed;

    writeFile(getCheckpointFilename(checkpointId), j);
    lastCheckpointId = checkpointId;
    
    NS_LOG_LOGIC("\n" << checkpointBaseName << " - CHECKPOINT CRIADO. ID: " << lastCheckpointId << "\n");
}

void CheckpointHelper::confirmCheckpoint(int checkpointId){
    NS_LOG_FUNCTION(this);

    //nlohmann::json j = to_json(app);
    json j = readCheckpoint(checkpointId);
    j["confirmed"] = true;

    removeCheckpoint(checkpointId);
    writeFile(getCheckpointFilename(checkpointId), j);
    
    NS_LOG_LOGIC("\n" << checkpointBaseName << " - CHECKPOINT CONFIRMADO. ID: " << lastCheckpointId << "\n");
}

void CheckpointHelper::removeCheckpoint(int checkpointId){
    NS_LOG_FUNCTION(this);

    string filename = getCheckpointFilename(checkpointId);
    std::remove(filename.c_str());

    if (checkpointId == lastCheckpointId){
        //Reseta o valor do último checkpoint para forçar seu recarregamento
        lastCheckpointId = 0;
    }
}  

void CheckpointHelper::writeFile(string filename, nlohmann::json j){
    NS_LOG_FUNCTION(this);

    string content = getFileContent(filename);

    // Create and open a text file
    ofstream logFile(filename);

    logFile << content << j;

    logFile.close();
}

string CheckpointHelper::getFileContent(string filename)
{
    NS_LOG_FUNCTION(this);

    ifstream t(filename);
    stringstream buffer;
    buffer << t.rdbuf();

    return buffer.str();
}

json CheckpointHelper::readCheckpoint(int index)
{   
    NS_LOG_FUNCTION(this);
    string c = getFileContent(getCheckpointFilename(index));
    return json::parse(c);
}

json CheckpointHelper::readLastCheckpoint()
{
    NS_LOG_FUNCTION(this);

    string content = getFileContent(getCheckpointFilename(getLastCheckpointId()));

    /*if (content.length() == 0){
        content = getFileContent(getCheckpointFilename(getLastCheckpointId()-1));
    }*/

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

    return maxNumber;
    
}

int CheckpointHelper::getPreviousCheckpointId(int checkpointId) {
    DIR* dir;
    struct dirent* entry;
    std::vector<int> ids;

    dir = opendir(CHECKPOINTS_FOLDER.c_str());
    
    if (!dir) {
        NS_ABORT_MSG("Erro ao abrir diretório dos checkpoints");
        return -1;
    }

    while ((entry = readdir(dir)) != nullptr) {
        string filename = entry->d_name;

        // Verifica se termina com ".json"
        if (filename.length() >= 5 && filename.substr(filename.length() - 5) == ".json") {
            string nameWithoutExt = filename.substr(0, filename.length() - 5);

            size_t pos = nameWithoutExt.find_last_of('-');
            if (pos != string::npos && pos + 1 < nameWithoutExt.length()) {
                string idPart = nameWithoutExt.substr(pos + 1);
                
                bool isNumber = !idPart.empty() && std::all_of(idPart.begin(), idPart.end(), ::isdigit);
                
                if (isNumber) {
                    int id = atoi(idPart.c_str());
                    
                    if (id < checkpointId) {
                        ids.push_back(id);
                    }
                }
            }
        }
    }

    closedir(dir);

    if (ids.empty()) {
        return -1; // Nenhum ID anterior encontrado
    }

    return *max_element(ids.begin(), ids.end());
}

void to_json(json& j, const CheckpointHelper& obj) {
    NS_LOG_FUNCTION("CheckpointHelper::to_json");

    j = json{
        {"checkpointBaseName", obj.checkpointBaseName}, 
        {"lastCheckpointId", obj.lastCheckpointId}
    };
}

void from_json(const json& j, CheckpointHelper& obj) {
    NS_LOG_FUNCTION("CheckpointHelper::from_json");

    j.at("checkpointBaseName").get_to(obj.checkpointBaseName);
    j.at("lastCheckpointId").get_to(obj.lastCheckpointId);
}

} // namespace ns3
