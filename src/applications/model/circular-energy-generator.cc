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

#include "circular-energy-generator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CircularEnergyGenerator");

NS_OBJECT_ENSURE_REGISTERED(CircularEnergyGenerator);

TypeId
CircularEnergyGenerator::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CircularEnergyGenerator")
            .SetParent<EnergyGenerator>()
            .SetGroupName("Sensor");

    return tid;
}

CircularEnergyGenerator::CircularEnergyGenerator(){
    NS_LOG_FUNCTION(this);
}

CircularEnergyGenerator::~CircularEnergyGenerator()
{
    NS_LOG_FUNCTION(this);
}

double CircularEnergyGenerator::getValue() {
    double v = values[i % std::size(values)];
    i++;
    return v;
}

void to_json(json& j, const CircularEnergyGenerator& obj) {
    to_json(j, static_cast<const EnergyGenerator&>(obj));
    j["i"] = obj.i;
}

void from_json(const json& j, CircularEnergyGenerator& obj) {
    j.at("i").get_to(obj.i);
}

} // Namespace ns3
