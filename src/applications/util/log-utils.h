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

#ifndef LOG_UTILS_H
#define LOG_UTILS_H

#include "ns3/core-module.h"
#include "ns3/log.h"
#include "ns3/message-data.h"
#include "ns3/udp-helper.h"

using namespace ns3;

namespace utils
{

/** 
 * Registra, em log, o recebimento de uma mensagem.
 * @param nodeName nome do nó que recebeu a mensagem.
 * @param md mensagem recebida.
 */
void logMessageReceived(string nodeName, Ptr<MessageData> md);

/** 
 * Registra, em log, o envio de uma mensagem normal de negócio do sistema.
 * @param nodeName nome do nó que enviou a mensagem.
 * @param md mensagem enviada.
 */
void logRegularMessageSent(string nodeName, Ptr<MessageData> md);

/** 
 * Registra, em log, o envio de uma mensagem normal de negócio do sistema.
 * @param nodeName nome do nó que enviou a mensagem.
 * @param md mensagem enviada.
 */
void logRegularMessageSent(string nodeName, Ptr<MessageData> md, uint64_t m_seq);

/** 
 * Registra, em log, o envio de uma mensagem de comando.
 * 
 * @param nodeName nome do nó que enviou a mensagem.
 * @param md mensagem enviada.
 */
//void logCommandSent(string nodeName, Ptr<MessageData> md);

} // namespace utils

#endif /* LOG_UTILS_H */
