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

#include "log-utils.h"

NS_LOG_COMPONENT_DEFINE("LogUtils");

namespace utils
{

void logMessageReceived(string nodeName, Ptr<MessageData> md, bool replay){
    NS_LOG_FUNCTION("LogUtils::logMessageReceived" << md);

    if (md->GetCommand() == REQUEST_VALUE && !replay)
        //Apenas para fins de organização, dá uma quebra de linha
        NS_LOG_INFO("");

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << nodeName 
                                << (replay ? " registrou o recebimento de " : " recebeu ")
                                << md->GetSize() << " bytes de "
                                << InetSocketAddress::ConvertFrom(md->GetFrom()).GetIpv4() << " porta "
                                << InetSocketAddress::ConvertFrom(md->GetFrom()).GetPort()
                                << " (Número de Sequência: " << md->GetSequenceNumber()
                                << ", UId: " << md->GetUid() 
                                << (md->GetCommand() != REQUEST_VALUE && md->GetCommand() != RESPONSE_VALUE
                                    && md->GetCommand() != ACKNOWLEDGEMENT_COMMAND ? 
                                                ", Comando: " + md->GetCompleteCommand()
                                                : "")
                                << (md->GetCommand() == ACKNOWLEDGEMENT_COMMAND ?
                                                ", Comando : " + md->GetCommand() + " " + to_string(md->GetData()) : "")
                                << (md->GetCommand() == RESPONSE_VALUE ? 
                                                ", m_seq: " + to_string(md->GetData()) 
                                                : "")
                                << (!md->GetPiggyBackedInfo().empty() ? ", " + md->GetPiggyBackedInfo() : "")
                                << ")");
}

void logRegularMessageSent(string nodeName, Ptr<MessageData> md, bool replay){

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << nodeName 
                            << (replay ? " registrou o envio de " : " enviou ")
                            << md->GetSize()
                            << " bytes para " << InetSocketAddress::ConvertFrom(md->GetTo()).GetIpv4()
                            << " porta " << InetSocketAddress::ConvertFrom(md->GetTo()).GetPort()
                            << " (Número de Sequência: " << md->GetSequenceNumber()
                            << ", UId: " << md->GetUid() 
                            << (md->GetCommand() != REQUEST_VALUE && md->GetCommand() != RESPONSE_VALUE ? 
                                            ", Comando: " + md->GetCommand() : "")
                            << (md->GetCommand() != REQUEST_VALUE && md->GetCommand() != RESPONSE_VALUE 
                                    && md->GetCommand() != ACKNOWLEDGEMENT_COMMAND && md->GetData() > 0 ? 
                                            " " + to_string(md->GetData()) : "")
                            << (md->GetCommand() == ACKNOWLEDGEMENT_COMMAND ?
                                            " " + to_string(md->GetData()) : "")
                            << (md->GetCommand() == RESPONSE_VALUE ? (", m_seq: " + to_string(md->GetData())) : "")
                            << (!md->GetPiggyBackedInfo().empty() ? ", " + md->GetPiggyBackedInfo() : "")
                            << ")"); 

}

// void logRegularMessageSent(string nodeName, Ptr<MessageData> md, uint64_t m_seq){

//     NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << nodeName << " enviou " << md->GetSize()
//                             << " bytes para " << InetSocketAddress::ConvertFrom(md->GetTo()).GetIpv4()
//                             << " porta " << InetSocketAddress::ConvertFrom(md->GetTo()).GetPort()
//                             << " (Número de Sequência: " << md->GetSequenceNumber()
//                             << ", UId: " << md->GetUid() 
//                             << ", m_seq: " << m_seq << ")"); 

// }

/*void logCommandSent(string nodeName, Ptr<MessageData> md){

    NS_LOG_INFO("Aos " << Simulator::Now().As(Time::S) << ", " << nodeName
                    << " enviou para "
                    << InetSocketAddress::ConvertFrom(md->GetTo()).GetIpv4()
                    << " porta " << InetSocketAddress::ConvertFrom(md->GetTo()).GetPort()
                    << " o seguinte comando: " << md->GetCommand() << " "
                    << (md->GetData() > 0 ? to_string(md->GetData()) : ""));

}*/

} // Namespace utils
