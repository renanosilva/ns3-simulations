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

#ifndef BATTERY_H
#define BATTERY_H

#include <ns3/nstime.h>
#include "ns3/object.h"
#include "ns3/type-id.h"
#include <ns3/assert.h>
#include <ns3/double.h>
#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/trace-source-accessor.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace ns3
{

class Battery : public Object
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    Battery();
    ~Battery() override;

    double getMaxCapacity(); 
    double getRemainingEnergy();
    double getBatteryPercentage(); //obtém o percentual de energia disponível atualmente na bateria
    
    void decrementEnergy(double amount); //descarrega a bateria, de acordo com o valor informado
    void rechargeEnergy(double amount); //recarrega a bateria, de acordo com o valor informado

    //Especifica como deve ser feita a conversão desta classe em JSON
    friend void to_json(json& j, const Battery& obj);

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    friend void from_json(const json& j, Battery& obj);

  private:

    double maxCapacity; //Capacidade máxima da bateria
    double remainingEnergy; //Energia restante da bateria
    
};

} // namespace ns3

#endif /* UDP_ECHO_SERVER_H */
