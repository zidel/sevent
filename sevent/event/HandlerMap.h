#pragma once

/** \namespace sevent::event
 *
 * sevent::socket has a single handler for all events, this namespace adds
 * event-forwarding to multiple handlers.
 *
 * \section Example
 *
 * \include EventExample.cc
 * */


#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include "sevent/socket.h"
#include "Event.h"
#include "EventId.h"

namespace sevent
{
    namespace event
    {
        class HandlerMap;
        typedef boost::shared_ptr<HandlerMap> HandlerMap_ptr;

        /** A map of event handlers to make it easier to route events. */
        class HandlerMap
        {
            public:
                typedef socket::Facade::allEventsHandler_t handler_t;
            private:
                struct Handler
                {
                    handler_t handler;
                };
                typedef boost::shared_ptr<Handler> Handler_ptr;

            public:
                HandlerMap();
                virtual ~HandlerMap();
                static HandlerMap_ptr make();
            public:
                bool contains(eventid_t::value_typeref eventid);
                void addEventHandler(eventid_t::value_typeref eventid,
                                     handler_t handler);
                void triggerEvent(socket::Facade_ptr facade,
                                  socket::Session_ptr session,
                                  Event_ptr event);

            private:
                boost::unordered_map<eventid_t::value_type, Handler_ptr> _handlers;
                boost::mutex _lock;
        };
        

        /** A "all events handler" which forwards incoming events to the
         * handlers registered with the eventHandlerMap.
         *
         * This is intentionally left really simple because one should
         * create a custom allEventsHandler in all situations except
         * testing/examples to keep error handling in the application,
         * instead of just having the application crash.
         *
         * \throws std::runtime_error If the eventid is not in the map.
         * */
        void simpleAllEventsHandler(event::HandlerMap_ptr eventHandlerMap,
                                     socket::Facade_ptr facade,
                                     socket::Session_ptr session,
                                     Event_ptr event);

    } // namespace event
} // namespace sevent
