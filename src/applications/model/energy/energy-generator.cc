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

#include "energy-generator.h"
using namespace std;

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EnergyGenerator");

NS_OBJECT_ENSURE_REGISTERED(EnergyGenerator);

TypeId
EnergyGenerator::GetTypeId()
{
    NS_LOG_FUNCTION("EnergyGenerator::GetTypeId()");

    static TypeId tid =
        TypeId("ns3::EnergyGenerator")
            .AddConstructor<EnergyGenerator>()
            .SetParent<Object>()
            .SetGroupName("Sensor");
    
    NS_LOG_FUNCTION("Fim do método");
    return tid;
}

EnergyGenerator::EnergyGenerator(){
    NS_LOG_FUNCTION(this);
    NS_LOG_FUNCTION("Fim do método");
}

EnergyGenerator::~EnergyGenerator()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_FUNCTION("Fim do método");
}

double EnergyGenerator::getValue(){
    return 0;
}

void to_json(json& j, const EnergyGenerator& obj) {
    NS_LOG_FUNCTION("EnergyGenerator::to_json");
    
    j = json{
        //até o momento não é necessário converter nada
    };

    NS_LOG_FUNCTION("Fim do método");
}

void from_json(const json& j, EnergyGenerator& obj) {
    //até o momento não é necessário converter nada
}

} // Namespace ns3
