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

#ifndef UTILS_H
#define UTILS_H

#include "ns3/core-module.h"
#include "ns3/ipv4-address.h"
#include <string>

using namespace ns3;
using namespace std;

namespace utils
{

/**
 * Converte o vetor de endereços informado para o formato 'IP;IP;...' (string).
 */
string convertAddressesToString(vector<Ipv4Address> addresses);

} // namespace utils

#endif /* UTILS_H */
