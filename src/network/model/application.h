/*
 * Copyright (c) 2006 Georgia Tech Research Corporation
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
 * Author: George F. Riley<riley@ece.gatech.edu>
 */

#ifndef APPLICATION_H
#define APPLICATION_H

#include "node.h"

#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/ptr.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace ns3
{

class Node;

/**
 * \addtogroup applications Applications
 *
 * Class ns3::Application can be used as a base class for ns3 applications.
 * Applications are associated with individual nodes.  Each node
 * holds a list of references (smart pointers) to its applications.
 *
 * Conceptually, an application has zero or more ns3::Socket
 * objects associated with it, that are created using the Socket
 * creation API of the Kernel capability.  The Socket object
 * API is modeled after the
 * well-known BSD sockets interface, although it is somewhat
 * simplified for use with ns3.  Further, any socket call that
 * would normally "block" in normal sockets will return immediately
 * in ns3.  A set of "upcalls" are defined that will be called when
 * the previous blocking call would normally exit.  THis is documented
 * in more detail Socket class in socket.h.
 *
 * The main purpose of the base class application public API is to
 * provide a uniform way to start and stop applications.
 */

/**
 * \brief The base class for all ns3 applications
 *
 */
class Application : public Object
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    Application();
    ~Application() override;

    /**
     * \brief Specify application start time
     * \param start Start time for this application,
     *        relative to the current simulation time.
     *
     * Applications start at various times in the simulation scenario.
     * This method specifies when the application should be
     * started.  The application subclasses should override the
     * private "StartApplication" method defined below, which is called at the
     * time specified, to cause the application to begin.
     */
    void SetStartTime(Time start);

    /**
     * \brief Specify application stop time
     * \param stop Stop time for this application, relative to the
     *        current simulation time.
     *
     * Once an application has started, it is sometimes useful
     * to stop the application.  This method specifies when an
     * application is to stop.  The application subclasses should override
     * the private StopApplication method, to be notified when that
     * time has come.
     */
    void SetStopTime(Time stop);

    /**
     * \returns the Node to which this Application object is attached.
     */
    Ptr<Node> GetNode() const;

    /**
     * \param node the node to which this Application object is attached.
     */
    void SetNode(Ptr<Node> node);

    /**
     * \brief Assign a fixed random variable stream number to the random variables
     * used by this Application object.
     *
     * \param stream first stream index to use
     * \return the number of stream indices assigned by this Application object
     */
    virtual int64_t AssignStreams(int64_t stream);

    /**
     * \brief Common callback signature for packet delay and address.
     *
     * \param delay The packet delay.
     * \param from The source socket address associated with the packet,
     *             indicating the packet's origin.
     */
    typedef void (*DelayAddressCallback)(const Time& delay, const Address& from);

    /**
     * \brief Common signature used by callbacks to application's state
     *        transition trace source.
     * \param oldState The name of the previous state.
     * \param newState The name of the current state.
     */
    typedef void (*StateTransitionCallback)(const std::string& oldState,
                                            const std::string& newState);

    /** 
     * Função para converter a classe em JSON. Irá escrever os atributos das classes filhas.
     * NÃO ALTERAR A ASSINATURA DESTE MÉTODO!
     */
    virtual json to_json() const;

    /** 
     * Função para converter JSON nesta classe.
     * NÃO MEXER NA ASSINATURA DESTE MÉTODO!
     */
    virtual void from_json(const json& j);

  private:
    /**
     * \brief Application specific startup code
     *
     * The StartApplication method is called at the start time specified by Start
     * This method should be overridden by all or most application
     * subclasses.
     */
    virtual void StartApplication();

    /**
     * \brief Application specific shutdown code
     *
     * The StopApplication method is called at the stop time specified by Stop
     * This method should be overridden by all or most application
     * subclasses.
     */
    virtual void StopApplication();

  protected:
    void DoDispose() override;
    void DoInitialize() override;

    Ptr<Node> m_node;     //!< The node that this application is installed on
    Time m_startTime;     //!< The simulation time that the application will start
    Time m_stopTime;      //!< The simulation time that the application will end
    EventId m_startEvent; //!< The event that will fire at m_startTime to start the application
    EventId m_stopEvent;  //!< The event that will fire at m_stopTime to end the application

};

} // namespace ns3

#endif /* APPLICATION_H */
