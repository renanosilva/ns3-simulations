/*
 * Copyright (c) 2010 Andrea Sacco
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
 * Author: Andrea Sacco <andrea.sacco85@gmail.com>
 */

#include "sensor-energy-model.h"

#include "energy-source.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"

namespace ns3
{
namespace energy
{

NS_LOG_COMPONENT_DEFINE("SensorEnergyModel");
NS_OBJECT_ENSURE_REGISTERED(SensorEnergyModel);

TypeId
SensorEnergyModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SensorEnergyModel")
                            .SetParent<DeviceEnergyModel>()
                            .SetGroupName("Energy")
                            .AddConstructor<SensorEnergyModel>()
                            .AddTraceSource("TotalEnergyConsumption",
                                            "Total energy consumption of the radio device.",
                                            MakeTraceSourceAccessor(
                                                &SensorEnergyModel::m_totalEnergyConsumption),
                                            "ns3::TracedValueCallback::Double");
    return tid;
}

SensorEnergyModel::SensorEnergyModel()
{
    NS_LOG_FUNCTION(this);
    m_lastUpdateTime = Seconds(0.0);
    m_actualCurrentA = 0.0;
    m_source = nullptr;
}

SensorEnergyModel::~SensorEnergyModel()
{
    NS_LOG_FUNCTION(this);
}

void
SensorEnergyModel::SetEnergySource(Ptr<EnergySource> source)
{
    NS_LOG_FUNCTION(this << source);
    NS_ASSERT(source);
    m_source = source;
}

void
SensorEnergyModel::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this << node);
    NS_ASSERT(node);
    m_node = node;
}

Ptr<Node>
SensorEnergyModel::GetNode() const
{
    NS_LOG_FUNCTION(this);
    return m_node;
}

double
SensorEnergyModel::GetTotalEnergyConsumption() const
{
    NS_LOG_FUNCTION(this);
    Time duration = Simulator::Now() - m_lastUpdateTime;

    double energyToDecrease = 0.0;
    double supplyVoltage = m_source->GetSupplyVoltage();
    energyToDecrease = duration.GetSeconds() * m_actualCurrentA * supplyVoltage;

    m_source->UpdateEnergySource();

    return m_totalEnergyConsumption + energyToDecrease;
}

void
SensorEnergyModel::SetCurrentA(double current)
{
    bool currentModified = m_actualCurrentA != current;

    NS_LOG_FUNCTION(this << current);
    Time duration = Simulator::Now() - m_lastUpdateTime;

    double energyToDecrease = 0.0;
    double supplyVoltage = m_source->GetSupplyVoltage();
    energyToDecrease = duration.GetSeconds() * m_actualCurrentA * supplyVoltage;

    // update total energy consumption
    m_totalEnergyConsumption += energyToDecrease;
    // update last update time stamp
    m_lastUpdateTime = Simulator::Now();
    // update the current drain
    m_actualCurrentA = current;
    // notify energy source
    m_source->UpdateEnergySource();

    //se o valor de corrente foi modificado
    if (currentModified){
        //após 0,02 segundos, o consumo volta a zero
        Simulator::Schedule(Seconds(0.02),
                                &SensorEnergyModel::SetCurrentA,
                                this,
                                0.0);
    }
}

void
SensorEnergyModel::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_source = nullptr;
}

double
SensorEnergyModel::DoGetCurrentA() const
{
    NS_LOG_FUNCTION(this);
    return m_actualCurrentA;
}

} // namespace energy
} // namespace ns3
