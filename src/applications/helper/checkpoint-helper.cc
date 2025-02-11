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

using namespace std;

namespace ns3
{

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

string CheckpointHelper::getCheckpointFilename(int i){
    return CHECKPOINTS_FOLDER + checkpointBaseName + "-" + to_string(i) + ".json";
}

string CheckpointHelper::getLogFilename(int i){
    return CHECKPOINTS_FOLDER + getLogBasename() + "-" + to_string(i) + ".json";
}

void CheckpointHelper::removeAllCheckpointsAndLogs()
{
    cleanDirectory(CHECKPOINTS_FOLDER);
}

bool CheckpointHelper::cleanDirectory(const std::string &path){
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
    
    vector<string> arquivos;
    DIR* dir = opendir(pasta.c_str());
    
    if (dir == nullptr) {
        cerr << "Erro ao abrir o diretório!" << std::endl;
        return arquivos;
    }

    struct dirent* entry;
    
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG) { // Se for um arquivo regular
            if (std::strstr(entry->d_name, padrao.c_str()) != nullptr) {
                arquivos.push_back(entry->d_name);
            }
        }
    }

    closedir(dir);
    return arquivos;
}

void CheckpointHelper::writeCheckpoint(string data){
    nlohmann::json j;
    j.push_back(data);

    writeFile(getCheckpointFilename(counter), j);
    counter++;
}

void CheckpointHelper::writeCheckpoint(Application *app){
    //nlohmann::json j = to_json(app);
    json j = app->to_json();

    writeFile(getCheckpointFilename(counter), j);
    counter++;

}  

void CheckpointHelper::skipCheckpoint(){
    counter++;
}

void CheckpointHelper::writeLog(string data){
    writeFile(getLogFilename(counter), data);
}

void CheckpointHelper::writeFile(string filename, nlohmann::json j){
    
    string content = getFileContent(filename);

    // Create and open a text file
    ofstream logFile(filename);

    logFile << content << j;

    logFile.close();
}

string CheckpointHelper::getFileContent(string filename)
{
    ifstream t(filename);
    stringstream buffer;
    buffer << t.rdbuf();

    return buffer.str();
}

json CheckpointHelper::readCheckpoint(int index)
{   
    string c = getFileContent(getCheckpointFilename(index));
    return json::parse(c);
}

json CheckpointHelper::readLastCheckpoint()
{
    string content = getFileContent(getCheckpointFilename(getLastCounter()));

    if (content.length() == 0){
        content = getFileContent(getCheckpointFilename(getLastCounter()-1));
    }

    return json::parse(content);
}

string CheckpointHelper::getCheckpointBasename(){
    return checkpointBaseName;
}

string CheckpointHelper::getLogBasename(){
    return checkpointBaseName + "-log";
}

int CheckpointHelper::getLastCounter(){
    if (counter != 0){
        cout << "\n" << counter << "\n";
        return counter;
    }

    vector<string> nomes = listFiles(CHECKPOINTS_FOLDER, checkpointBaseName);
    string nome = nomes[nomes.size() - 1]; //pegando o nome do último arquivo

    //obtendo o contador do checkpoint presente no nome do arquivo do checkpoint

    // Expressão regular para capturar o número antes de ".json"
    regex re("^(.*)-node-(\\d+)-(\\d+)\\.json$");
    smatch match;
    
    if (regex_search(nome, match, re)) {
        // O número antes de ".json" será o terceiro grupo capturado
        string c = match[3];
        counter = stoi(c);
    } 

    return counter;
}

void to_json(json& j, const CheckpointHelper& obj) {
    j = json{
        {"checkpointBaseName", obj.checkpointBaseName}, 
        {"counter", obj.counter}
    };
}

void from_json(const json& j, CheckpointHelper& obj) {
    j.at("checkpointBaseName").get_to(obj.checkpointBaseName);
    j.at("counter").get_to(obj.counter);
}

} // namespace ns3
