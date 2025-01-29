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

#include "fixed-energy-generator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("FixedEnergyGenerator");

NS_OBJECT_ENSURE_REGISTERED(FixedEnergyGenerator);

TypeId
FixedEnergyGenerator::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::FixedEnergyGenerator")
            .SetParent<EnergyGenerator>()
            .SetGroupName("Sensor")
            .AddAttribute("value",
                          "Valor fixo de energia a ser gerada",
                          DoubleValue(40), //valor inicial
                          MakeDoubleAccessor(&FixedEnergyGenerator::value),
                          MakeDoubleChecker<double>());
    return tid;
}

FixedEnergyGenerator::FixedEnergyGenerator(double fixedValue){
    NS_LOG_FUNCTION(this);

    value = fixedValue;
}

FixedEnergyGenerator::~FixedEnergyGenerator()
{
    NS_LOG_FUNCTION(this);
}

double FixedEnergyGenerator::getValue() {
    return value;
}

} // Namespace ns3
