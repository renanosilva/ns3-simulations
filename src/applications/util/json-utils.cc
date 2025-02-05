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

#include "json-utils.h"
#include "ns3/fixed-energy-generator.h"
#include "ns3/circular-energy-generator.h"
#include "ns3/sync-predefined-times-checkpoint.h"

namespace ns3
{

json energyGeneratorToJson(json j, EnergyGenerator *eg) {
    FixedEnergyGenerator* feg = dynamic_cast<FixedEnergyGenerator*>(eg);

    if (feg) {
        j["energyGenerator"] = *feg;
    } else {
        CircularEnergyGenerator* ceg = dynamic_cast<CircularEnergyGenerator*>(eg);

        if (ceg){
            j["energyGenerator"] = *ceg;
        } else {
            j["energyGenerator"] = *eg;
        }
    }

    return j;
}

json checkpointStrategyToJson(json j, CheckpointStrategy *cs) {
    SyncPredefinedTimesCheckpoint* sptc = dynamic_cast<SyncPredefinedTimesCheckpoint*>(cs);

    if (sptc) {
        j["checkpointStrategy"] = *sptc;
    } else {
        j["checkpointStrategy"] = *cs;
    }

    return j;
}

json timeToJson(json j, string propertyName, Time t){
    j[propertyName] = t.GetTimeStep();
    return j;
}

/*json BatteryNodeApp::socketToJson(json j) const {
    UdpSocket* us = dynamic_cast<UdpSocket*>(m_socket.operator->());

    if (us) {
        j["m_socket"] = *us;
    } else {
        j["m_socket"] = *m_socket;
    }

    return j;
}*/

} // Namespace ns3
