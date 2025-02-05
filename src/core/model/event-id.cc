/*
 * Copyright (c) 2005 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "event-id.h"

#include "event-impl.h"
#include "log.h"
#include "simulator.h"

/**
 * \file
 * \ingroup events
 * ns3::EventId implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EventId");

EventId::EventId()
    : m_eventImpl(nullptr),
      m_ts(0),
      m_context(0),
      m_uid(0)
{
    NS_LOG_FUNCTION(this);
}

EventId::EventId(const Ptr<EventImpl>& impl, uint64_t ts, uint32_t context, uint32_t uid)
    : m_eventImpl(impl),
      m_ts(ts),
      m_context(context),
      m_uid(uid)
{
    NS_LOG_FUNCTION(this << impl << ts << context << uid);
}

void
EventId::Cancel()
{
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(*this);
}

void
EventId::Remove()
{
    NS_LOG_FUNCTION(this);
    Simulator::Remove(*this);
}

bool
EventId::IsExpired() const
{
    NS_LOG_FUNCTION(this);
    return Simulator::IsExpired(*this);
}

bool
EventId::IsPending() const
{
    NS_LOG_FUNCTION(this);
    return !IsExpired();
}

bool
EventId::IsRunning() const
{
    return IsPending();
}

EventImpl*
EventId::PeekEventImpl() const
{
    NS_LOG_FUNCTION(this);
    return PeekPointer(m_eventImpl);
}

uint64_t
EventId::GetTs() const
{
    NS_LOG_FUNCTION(this);
    return m_ts;
}

uint32_t
EventId::GetContext() const
{
    NS_LOG_FUNCTION(this);
    return m_context;
}

uint32_t
EventId::GetUid() const
{
    NS_LOG_FUNCTION(this);
    return m_uid;
}

void to_json(json& j, const EventId& obj) {
    j = json{
        {"m_context", obj.m_context}, 
        {"m_eventImpl", *obj.m_eventImpl.operator->()},
        {"m_ts", obj.m_ts},
        {"m_uid", obj.m_uid}
    };
}

void from_json(const json& j, EventId& obj) {
    j.at("m_context").get_to(obj.m_context);
    //j.at("m_eventImpl").get_to(obj.m_eventImpl.operator->());
    j.at("m_ts").get_to(obj.m_ts);
    j.at("m_uid").get_to(obj.m_uid);
}

} // namespace ns3
