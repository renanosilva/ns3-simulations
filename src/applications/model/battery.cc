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
                          DoubleValue(2500.), 
                          MakeDoubleAccessor(&Battery::remainingEnergy),
                          MakeDoubleChecker<double>());
    return tid;
}

Battery::Battery(){
    NS_LOG_FUNCTION(this);

    maxCapacity = 2500.;
    remainingEnergy = maxCapacity;
}

Battery::~Battery()
{
    NS_LOG_FUNCTION(this);
}

double Battery::getMaxCapacity(){
    return maxCapacity;
}

double Battery::getRemainingEnergy(){
    return remainingEnergy;
}

double Battery::getBatteryPercentage(){
    return (remainingEnergy/maxCapacity)*100;
}

void Battery::rechargeEnergy(double amount){
    double newRemainingEnergy = remainingEnergy + amount;
    remainingEnergy = min(maxCapacity, newRemainingEnergy);
    
    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", energia gerada: " << amount 
                << ". Energia restante: " << to_string(remainingEnergy));
}

void Battery::decrementEnergy(double amount){
    NS_ASSERT_MSG(amount <= remainingEnergy, "Operação não pôde ser concluída! Quantidade de energia insuficiente.");
    
    double newRemainingEnergy = remainingEnergy - amount;
    remainingEnergy = max(0., newRemainingEnergy);
    
    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << " Energia consumida: " << amount << 
                ". Energia restante: " << to_string(remainingEnergy));
}

} // Namespace ns3
