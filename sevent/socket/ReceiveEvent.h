#pragma once
#include <stdint.h>
#include <ostream>
#include "MutableBuffer.h"

namespace sevent
{
    namespace socket
    {

        class ReceiveEvent
        {
            public:
                typedef unsigned eventId_t;
                typedef uint32_t dataSize_t;
            public:
                ReceiveEvent(eventId_t eventid, MutableBufferVector_ptr datavector_);
                virtual ~ReceiveEvent();
                eventId_t eventid() const;
                MutableBuffer_ptr popBack();
                MutableBuffer_ptr first();
            public:
                MutableBufferVector_ptr datavector;
            private:
                eventId_t _eventid;
                char* _data;
                dataSize_t _dataSize;
        };

        std::ostream& operator<<(std::ostream& out, const ReceiveEvent& event);
    } // namespace socket
} // namespace sevent
