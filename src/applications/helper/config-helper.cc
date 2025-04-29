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

#include "config-helper.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ConfigHelper");
NS_OBJECT_ENSURE_REGISTERED(ConfigHelper);

TypeId ConfigHelper::GetTypeId() {
    static TypeId tid = TypeId("ConfigHelper")
        .SetParent<Object>()
        .SetGroupName("Helper");
    return tid;
}

// Construtor padrão
ConfigHelper::ConfigHelper() {

}

// Construtor que carrega o arquivo JSON
ConfigHelper::ConfigHelper(const string &filename) {
    LoadFile(filename);
}

ConfigHelper::~ConfigHelper() {

}

void ConfigHelper::LoadFile(const string &filename) {
    try {
        boost::property_tree::read_json(filename, propertyTree);
    } catch (const std::exception &e) {
        std::cerr << "Erro ao carregar o arquivo: " << e.what() << std::endl;
    }
}

int ConfigHelper::GetPropertyValuesCount(string property) {
    try {
        return propertyTree.get_child(property).size();
    } catch (const std::exception& e) {
        std::cerr << "Erro ao acessar battery-nodes: " << e.what() << std::endl;
        return 0; // Retorna 0 se a chave não existir ou houver erro
    }
}

vector<string> ConfigHelper::GetPropertyValues(const string& propertyPath) {
    vector<string> keys;
    
    try {
        // Obtém a subárvore correspondente ao caminho fornecido
        const auto& subtree = propertyTree.get_child(propertyPath);

        // Adiciona todas as chaves encontradas ao vetor
        for (const auto& node : subtree) {
            keys.push_back(node.first);
        }
    } catch (const std::exception& e) {
        std::cerr << "Erro ao acessar " << propertyPath << ": " << e.what() << std::endl;
    }
    
    return keys;
}

// Retorna os nomes dos nós que possuem um valor específico na chave "energy-generator"
vector<string> ConfigHelper::GetNodesWithRole(const string& role) {
    
    vector<string> matchingNodes;

    try {
        const auto& nodes = propertyTree.get_child("nodes");

        for (const auto& node : nodes) {
            const std::string& nodeName = node.first;
            const auto& properties = node.second;

            auto it = properties.find("role");
            
            if (it != properties.not_found()) {
                std::string roleValue = it->second.get_value<string>();
                if (roleValue == role) {
                    matchingNodes.push_back(nodeName);
                }
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Erro ao acessar os nós: " << e.what() << std::endl;
    }

    return matchingNodes;
}

int ConfigHelper::GetIntProperty(const string key){
    try {
        return propertyTree.get<int>(key);
    } catch (const std::exception &e) {
        std::cerr << "Erro ao ler propriedade: " << key << " - " << e.what() << std::endl;
        return 0;
    }
}

double ConfigHelper::GetDoubleProperty(const string key){
    try {
        return propertyTree.get<double>(key);
    } catch (const std::exception &e) {
        std::cerr << "Erro ao ler propriedade: " << key << " - " << e.what() << std::endl;
        return 0;
    }
}

double ConfigHelper::GetDoubleProperty(const string key, double defaultValue){
    try {
        return propertyTree.get<double>(key);
    } catch (const std::exception &e) {
        std::cerr << "Erro ao ler propriedade: " << key << " - " << e.what() 
            << ". Atribuindo valor padrão: " << defaultValue << "." << std::endl;
        return defaultValue;
    }
}

bool ConfigHelper::GetBoolProperty(const string key, bool defaultValue){
    try {
        return propertyTree.get<bool>(key);
    } catch (const std::exception &e) {
        return defaultValue;
    }
}

string ConfigHelper::GetStringProperty(const string key){
    try {
        return propertyTree.get<string>(key);
    } catch (const std::exception &e) {
        std::cerr << "Erro ao ler propriedade: " << key << " - " << e.what() << std::endl;
        return "";
    }
}

} // namespace ns3
