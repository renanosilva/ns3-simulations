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

#ifndef CIRCULAR_ENERGY_GENERATOR_H
#define CIRCULAR_ENERGY_GENERATOR_H

#include "ns3/object.h"
#include "ns3/type-id.h"
#include "energy-generator.h"
#include <ns3/double.h>

namespace ns3
{

/** Gera valores pré-estabelecidos para uma bateria. */
class CircularEnergyGenerator : public EnergyGenerator
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    CircularEnergyGenerator();
    ~CircularEnergyGenerator() override;

    virtual double getValue() override; 

    //Especifica como deve ser feita a conversão desta classe em JSON
    friend void to_json(json& j, const CircularEnergyGenerator& obj);

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    friend void from_json(const json& j, CircularEnergyGenerator& obj);

  private:

    /** Valores de energia a serem gerados */
    double values[60] = {0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0,
                        100.0, 110.0, 120.0, 130.0, 140.0, 150.0, 160.0, 170.0, 180.0, 190.0,
                        200.0, 210.0, 220.0, 230.0, 240.0, 250.0, 250.0, 250.0, 250.0, 250.0,
                        240.0, 230.0, 220.0, 210.0, 200.0, 190.0, 180.0, 170.0, 160.0, 150.0,
                        140.0, 130.0, 120.0, 110.0, 100.0, 90.0, 80.0, 70.0, 60.0, 50.0,
                        40.0, 30.0, 20.0, 10.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

    /** Contador do array, que indica qual valor de energia será fornecido no método getValue() */
    int i = 0;
};

} // namespace ns3

#endif /* UDP_ECHO_SERVER_H */
