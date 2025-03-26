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

#ifndef CONFIG_HELPER_H
#define CONFIG_HELPER_H

#include "ns3/core-module.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>

using namespace boost;
using namespace property_tree;
using namespace std;

namespace ns3
{

/**
 * \ingroup helper
 * \brief Classe que auxilia no processo de gerenciamento das configurações das simulações. 
 */
class ConfigHelper : public Object
{

private:

    ptree propertyTree;

public:

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /** Construtor padrão. */
    ConfigHelper();

    /** Construtor que carrega o arquivo de propriedades. */
    ConfigHelper(const string &filename);

    ~ConfigHelper() override;

    void LoadFile(const string &filename);

    /** Obtém a quantidade de valores associados a uma determinada propriedade (chave). */
    int GetPropertyValuesCount(string property);

    /** 
     * Obtém os valores associados a uma propriedade (chave). 
     * Útil para quando uma propriedade possui mais de um valor associado.
     * 
     * Ex.: 
     * "client-nodes": {
            "client-node-0":{
                "checkpoint-strategy": "SyncPredefinedTimesCheckpoint"
            },
            "client-node-1":{
                "checkpoint-strategy": "SyncPredefinedTimesCheckpoint"
            }
        }
     * 
     * */
    vector<string> GetPropertyValues(const string& propertyPath);

    /** 
     * Obtém o valor, do tipo inteiro, associado a uma propriedade.
     * O valor padrão, caso não seja possível obter o valor, é 0.
     */
    int GetIntProperty(const string key);

    /** 
     * Obtém o valor, do tipo double, associado a uma propriedade.
     * O valor padrão, caso não seja possível obter o valor, é 0.
     */
    double GetDoubleProperty(const string key);

    /** 
     * Obtém o valor, do tipo double, associado a uma propriedade.
     * O valor padrão, caso não seja possível obter o valor, é o 
     * do parâmetro defaultValue.
     */
    double GetDoubleProperty(const string key, double defaultValue);

    /** 
     * Obtém o valor, do tipo string, associado a uma propriedade.
     * O valor padrão, caso não seja possível obter o valor, é uma string
     * vazia.
     */
    string GetStringProperty(const string key);

};

} // namespace ns3

#endif /* CONFIG_HELPER_H */
