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

#include "event-impl.h"

#include "log.h"

/**
 * \file
 * \ingroup events
 * ns3::EventImpl definitions.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EventImpl");

EventImpl::~EventImpl()
{
    NS_LOG_FUNCTION(this);
}

EventImpl::EventImpl()
    : m_cancel(false)
{
    NS_LOG_FUNCTION(this);
}

void
EventImpl::Invoke()
{
    NS_LOG_FUNCTION(this);
    if (!m_cancel)
    {
        Notify();
    }
}

void
EventImpl::Cancel()
{
    NS_LOG_FUNCTION(this);
    m_cancel = true;
}

bool
EventImpl::IsCancelled()
{
    NS_LOG_FUNCTION(this);
    return m_cancel;
}

void to_json(json& j, const EventImpl& obj) {
    j = json{
        {"m_cancel", obj.m_cancel}
    };
}

void from_json(const json& j, EventImpl& obj) {
    j.at("m_cancel").get_to(obj.m_cancel);
}

} // namespace ns3
