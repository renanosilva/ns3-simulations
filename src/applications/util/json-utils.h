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

#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include "ns3/energy-generator.h"
#include "ns3/checkpoint-strategy.h"
#include "ns3/nstime.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace ns3
{

/** Converte um objeto gerador de energia em JSON. */
json energyGeneratorToJson(json j, EnergyGenerator *eg);

/** Converte um objeto estratégia de checkpoint em JSON. */
//json checkpointStrategyToJson(json j, CheckpointStrategy *cs);

//CheckpointStrategy* jsonToCheckpointStrategy(json j);

json timeToJson(json j, string propertyName, Time t);

Time jsonToTime(json j, string propertyName);

} // namespace ns3

#endif /* UDP_ECHO_SERVER_H */
