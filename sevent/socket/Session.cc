#include "Session.h"
#include <iostream>

namespace sevent
{
    namespace socket
    {

        Session::~Session()
        {
        }

        void Session::setDisconnectHandler(disconnectHandler_t disconnectHandler)
        {
            _disconnectHandler = disconnectHandler;
        }

        void Session::defaultAllEventsHandler(Session_ptr socketSession,
                ReceiveEvent eventData)
        {
            std::cout << "Event received by default all-events handler. "
                      << "Use Session::setAllEventsHandler to plug in your own."
                      << std::endl << "Event id: " << eventData.eventid() << std::endl
                      << "Event data size: " << eventData.dataSize() << std::endl
                      << "Event data: ";
            std::cout.write(eventData.data(), eventData.dataSize());
            std::cout << std::endl;
        }

        void Session::setAllEventsHandler(allEventsHandler_t allEventsHandler)
        {
            _allEventsHandler = allEventsHandler;
        }

    } // namespace socket
} // namespace sevent