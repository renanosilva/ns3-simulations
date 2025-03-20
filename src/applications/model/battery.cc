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

#include "battery.h"
#include "ns3/simulator.h"
#include <random>

using namespace std;

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Battery");
NS_OBJECT_ENSURE_REGISTERED(Battery);

TypeId
Battery::GetTypeId()
{
    NS_LOG_FUNCTION("Battery::GetTypeId()");

    static TypeId tid =
        TypeId("ns3::Battery")
            .AddConstructor<Battery>()
            .SetParent<Object>()
            .SetGroupName("Sensor")
            .AddAttribute("maxCapacity",
                          "Capacidade máxima da bateria",
                          DoubleValue(2500.), //valor inicial
                          MakeDoubleAccessor(&Battery::maxCapacity),
                          MakeDoubleChecker<double>())
            .AddAttribute("remainingEnergy",
                            "Energia restante da bateria",
                            DoubleValue(2500.), //valor inicial
                            MakeDoubleAccessor(&Battery::maxCapacity),
                            MakeDoubleChecker<double>());

    NS_LOG_FUNCTION("Fim do método");
    return tid;
}

Battery::Battery(){
    NS_LOG_FUNCTION(this);

    maxCapacity = 2500.;
    remainingEnergy = maxCapacity;

    NS_LOG_FUNCTION("Fim do método");
}

Battery::~Battery()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_FUNCTION("Fim do método");
}

double Battery::getMaxCapacity(){
    NS_LOG_FUNCTION(this);
    return maxCapacity;
}

double Battery::getRemainingEnergy(){
    NS_LOG_FUNCTION(this);
    return remainingEnergy;
}

double Battery::getBatteryPercentage(){
    NS_LOG_FUNCTION("this" << remainingEnergy << maxCapacity);
    return (remainingEnergy/maxCapacity)*100;
}

void Battery::rechargeEnergy(double amount){
    NS_LOG_FUNCTION(this);
    
    double newRemainingEnergy = remainingEnergy + amount;
    remainingEnergy = min(maxCapacity, newRemainingEnergy);
    
    NS_LOG_FUNCTION("Fim do método");
}

void Battery::decrementEnergy(double amount){
    NS_LOG_FUNCTION(this);
    
    NS_ASSERT_MSG(amount <= remainingEnergy, "Operação não pôde ser concluída! Quantidade de energia insuficiente.");
    
    double newRemainingEnergy = remainingEnergy - amount;
    remainingEnergy = max(0., newRemainingEnergy);
    
    NS_LOG_FUNCTION("Fim do método");
}

void to_json(json& j, const Battery& obj) {
    NS_LOG_FUNCTION("Battery::to_json");
    
    j = nlohmann::json{
        {"maxCapacity", obj.maxCapacity}, 
        {"remainingEnergy", obj.remainingEnergy}
    };

    NS_LOG_FUNCTION("Fim do método");
}

void from_json(const json& j, Battery& obj) {
    NS_LOG_FUNCTION("Battery::from_json");
    
    j.at("maxCapacity").get_to(obj.maxCapacity);
    j.at("remainingEnergy").get_to(obj.remainingEnergy);

    NS_LOG_FUNCTION("Fim do método");
}

} // Namespace ns3
