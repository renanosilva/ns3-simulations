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

#ifndef ENERGY_GENERATOR_H
#define ENERGY_GENERATOR_H

#include "ns3/object.h"
#include "ns3/type-id.h"
#include <ns3/double.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace ns3
{

/** Classe base para as classes que representam modelos de geração de energia. */
class EnergyGenerator : public Object
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    EnergyGenerator();
    ~EnergyGenerator() override;

    /** Obtém o valor de energia gerada. */
    virtual double getValue(); 

    //Especifica como deve ser feita a conversão desta classe em JSON
    friend void to_json(json& j, const EnergyGenerator& obj);

    //Especifica como deve ser feita a conversão de JSON em um objeto desta classe
    friend void from_json(const json& j, EnergyGenerator& obj);

};

} // namespace ns3

#endif
