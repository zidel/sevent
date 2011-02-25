#include "ReceiveEvent.h"

namespace sevent
{
    namespace socket
    {

        ReceiveEvent::ReceiveEvent(eventId_t eventid,
                                   MutableBufferVector_ptr datavector_) :
            _eventid(eventid), datavector(datavector_)
        {
        }

        ReceiveEvent::~ReceiveEvent()
        {
        }

        ReceiveEvent::eventId_t ReceiveEvent::eventid()
        {
            return _eventid;
        }

        ReceiveEvent::dataSize_t ReceiveEvent::firstDataSize()
        {
            return datavector->at(0)->size();
        }

        MutableBuffer_ptr ReceiveEvent::first()
        {
            return datavector->at(0);
        }
        MutableBuffer_ptr ReceiveEvent::popBack()
        {
            MutableBuffer_ptr last = datavector->back();
            datavector->pop_back();
            return last;
        }

    } // namespace socket
} // namespace sevent
