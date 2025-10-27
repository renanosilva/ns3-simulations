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

#include "utils.h"
#include "ns3/inet-socket-address.h"
#include "ns3/mac48-address.h"

NS_LOG_COMPONENT_DEFINE("Utils");

namespace utils
{

string convertAddressesToString(vector<Ipv4Address> addresses){
    ostringstream oss;
    
    for (size_t i = 0; i < addresses.size(); i++){
        
        if (i > 0)
            oss << ";"; // Separador entre IPs

        oss << addresses[i];

    }

    return oss.str();
}

string convertAddressToString(Ipv4Address address){
    ostringstream oss;
    oss << address;

    return oss.str();
}

string convertIpv4AddressToString(Ipv4Address address){
    return address.toString();
}

Ipv4Address convertStringToIpv4Address(string s){
    Ipv4Address ip(s.c_str());
    return ip;
}

Ipv4Address convertAddressToIpv4Address(Address a){
    InetSocketAddress inetAddr = InetSocketAddress::ConvertFrom(a);
    return inetAddr.GetIpv4();
}

} // Namespace utils
