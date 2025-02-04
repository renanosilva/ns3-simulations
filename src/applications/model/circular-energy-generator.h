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
    double values[60] = {0.0, 4.0, 6.0, 8.0, 10.0, 13.0, 13.0, 15.0, 15.0, 17.0,
                        18.0, 20.0, 23.0, 25.0, 25.0, 27.0, 28.0, 30.0, 30.0, 33.0,
                        35.0, 40.0, 40.0, 45.0, 50.0, 55.0, 60.0, 65.0, 70.0, 70.0,
                        70.0, 68.0, 66.0, 65.0, 65.0, 63.0, 62.0, 60.0, 55.0, 50.0,
                        45.0, 45.0, 40.0, 40.0, 35.0, 32.0, 30.0, 27.0, 25.0, 20.0,
                        17.0, 15.0, 10.0, 7.0, 5.0, 2.0, 0.0, 0.0, 0.0, 0.0};

    /** Contador do array, que indica qual valor de energia será fornecido no método getValue() */
    int i = 0;
};

} // namespace ns3

#endif /* UDP_ECHO_SERVER_H */
