#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestEvent
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_array.hpp>
#include <boost/function.hpp>
#include <boost/any.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <cstring>
#include <iostream>


struct ConstCharArray;
typedef boost::shared_ptr<ConstCharArray> ConstCharArray_ptr;
struct ConstCharArray
{
    const char* data;
    unsigned size;
    ConstCharArray(const char* data_, unsigned size_) : data(data_), size(size_) {}

    static ConstCharArray_ptr make(const char* data, unsigned size)
    {
        return boost::make_shared<ConstCharArray>(data, size);
    }
};

struct MutableCharArray
{
    char* data;
    unsigned size;
    MutableCharArray(char* data_, unsigned size_) : data(data_), size(size_) {}
};
typedef boost::shared_ptr<MutableCharArray> MutableCharArray_ptr;
typedef std::vector<MutableCharArray_ptr> MutableCharArrayVector;
typedef boost::shared_ptr<MutableCharArrayVector> MutableCharArrayVector_ptr;

typedef boost::function<ConstCharArray_ptr (boost::any& data)> serialize_t;
typedef boost::function<boost::any (MutableCharArray_ptr serialized)> deserialize_t;
struct SerializePair
{
    serialize_t serializeFunc;
    deserialize_t deserializeFunc;
    SerializePair(serialize_t serializeFunc_, deserialize_t deserializeFunc_) :
        serializeFunc(serializeFunc_), deserializeFunc(deserializeFunc_) {}
};


class Buffer;
typedef boost::shared_ptr<Buffer> Buffer_ptr;
class Buffer
{
    public:
        static Buffer_ptr make(boost::any anydata, serialize_t serializeFunc)
        {
            return boost::make_shared<Buffer>(anydata, serializeFunc);
        }

        static Buffer_ptr deserialize(MutableCharArray_ptr serialized,
                                      serialize_t serializeFunc,
                                      deserialize_t deserializeFunc)
        {
            boost::any anydata = deserializeFunc(serialized);
            return Buffer::make(anydata, serializeFunc);
        }

    public:
        Buffer(boost::any anydata, serialize_t serializeFunc) :
            _anydata(anydata), _serializeFunc(serializeFunc) {}

        ConstCharArray_ptr serialize()
        {
            return _serializeFunc(_anydata);
        }

        template<typename T> T data()
        {
            return boost::any_cast<T>(_anydata);
        }

    private:
        boost::any _anydata;
        serialize_t _serializeFunc;
};




typedef std::vector<Buffer_ptr> BufferVector;


template<typename T>
class Array
{
    public:
        typedef T value_type;
        typedef boost::shared_array<value_type> value_shared_array;
        value_shared_array sharedarr;
        unsigned size;
    public:
        Array(value_shared_array sharedarr_, unsigned size_) :
            sharedarr(sharedarr_), size(size_) {}
};
typedef Array<int> IntArray;
typedef boost::shared_ptr<IntArray> IntArray_ptr;


ConstCharArray_ptr serializeIntArray(boost::any& data)
{
    IntArray_ptr a = boost::any_cast<IntArray_ptr>(data);
    for(int i = 0; i < a->size; i++)
    {
        a->sharedarr[i] --;
    }
    return ConstCharArray::make(reinterpret_cast<const char*>(a->sharedarr.get()),
                                a->size*sizeof(int));
}

boost::any deserializeIntArray(MutableCharArray_ptr serialized)
{
    int* data = reinterpret_cast<int*>(serialized->data);
    IntArray_ptr arr = boost::make_shared<IntArray>(boost::shared_array<int>(data),
                                                    serialized->size/sizeof(int));
    return arr;
}
SerializePair IntSerializer(serializeIntArray, deserializeIntArray);


class Event;
typedef boost::shared_ptr<Event> Event_ptr;
class Event
{
    public:
        static Event_ptr make(unsigned eventid, Buffer_ptr first)
        {
            return boost::make_shared<Event>(eventid, first);
        }
        static Event_ptr make(unsigned eventid, MutableCharArrayVector_ptr serialized)
        {
            return boost::make_shared<Event>(eventid, serialized);
        }

    public:
        Event(unsigned eventid) :
            _eventid(eventid)
        {}

        Event(unsigned eventid, Buffer_ptr first) :
            _eventid(eventid)
        {
            _buffers.push_back(first);
        }

        Event(unsigned eventid, MutableCharArrayVector_ptr serialized) :
            _eventid(eventid), _buffers(serialized->size()), _serialized(serialized),
            _isSerialized(serialized->size(), true)
        {}

        template<typename T>
        T at(unsigned index, const SerializePair& serializer)
        {
            return buffer_at(index, serializer)->data<T>();
        }

        template<typename T>
        T first(const SerializePair& serializer)
        {
            return at<T>(0, serializer);
        }

        ConstCharArray_ptr serialize_at(int index)
        {
            return _buffers.at(index)->serialize();
        }

        void push_back(Buffer_ptr buffer)
        {
            boost::lock_guard<boost::mutex> lock(_lock);
            _buffers.push_back(buffer);
            _isSerialized.push_back(false);
        }

        bool isSerialized(int index)
        {
            return _serialized && _isSerialized[index];
        }

        unsigned size()
        {
            return _buffers.size();
        }

    private:
        void set_buffer(unsigned index, Buffer_ptr buffer)
        {
            _buffers[index] = buffer;
            _isSerialized[index] = false;
        }

        Buffer_ptr buffer_at(int index, const SerializePair& serializer)
        {
            boost::lock_guard<boost::mutex> lock(_lock);
            if(isSerialized(index))
            {
                MutableCharArray_ptr mutarr = _serialized->at(index);
                Buffer_ptr buffer = Buffer::deserialize(mutarr,
                                                        serializer.serializeFunc,
                                                        serializer.deserializeFunc);
                mutarr.reset(); // Free the MutableCharArray_ptr (data ownership is given to the deserializer)
                set_buffer(index, buffer);
            }
            return _buffers.at(index);
        }

    private:
        unsigned _eventid;
        BufferVector _buffers;
        MutableCharArrayVector_ptr _serialized;
        boost::dynamic_bitset<> _isSerialized;
        boost::mutex _lock;
};

struct EventFixture
{
    Buffer_ptr inputArray;

    EventFixture ()
    {
        boost::shared_array<int> intarr = boost::shared_array<int>(new int[2]);
        intarr[0] = 10;
        intarr[1] = 20;
        inputArray = Buffer::make(boost::make_shared<IntArray>(intarr, 2),
                                serializeIntArray);
    }

    ~EventFixture () {}
};

BOOST_FIXTURE_TEST_SUITE(BasicSuite, EventFixture)


BOOST_AUTO_TEST_CASE(TestLowLevelSerialization)
{
    ConstCharArray_ptr serialized = inputArray->serialize();

    // Simulate network transfer (copy the serialized data)
    char* receivedData = new char[serialized->size];
    std::memcpy(receivedData, serialized->data, serialized->size);
    MutableCharArray_ptr received = boost::make_shared<MutableCharArray>(receivedData,
                                                                         serialized->size);

    // Deserialize the "received data"
    Buffer_ptr output = Buffer::deserialize(received, serializeIntArray, deserializeIntArray);
    IntArray_ptr aOut = output->data<IntArray_ptr>();
    BOOST_REQUIRE_EQUAL(aOut->sharedarr[0], 9); // The array serializer reduce values by one..
    BOOST_REQUIRE_EQUAL(aOut->sharedarr[1], 19);
    BOOST_REQUIRE_EQUAL(aOut->size, 2);
}



BOOST_AUTO_TEST_CASE(TestEvent)
{
    Event_ptr eventIn = Event::make(1010, inputArray);
    BOOST_REQUIRE_EQUAL(eventIn->isSerialized(0), false);
    IntArray_ptr aOut = eventIn->first<IntArray_ptr>(IntSerializer);
    BOOST_REQUIRE_EQUAL(eventIn->isSerialized(0), false);
    BOOST_REQUIRE_EQUAL(aOut->sharedarr[0], 10);
    BOOST_REQUIRE_EQUAL(aOut->sharedarr[1], 20);
    BOOST_REQUIRE_EQUAL(aOut->size, 2);
}

BOOST_AUTO_TEST_CASE(TestEventSerialized)
{
    ConstCharArray_ptr serializedArray = inputArray->serialize();

    // Simulate network transfer (copy the serialized data)
    char* receivedData = new char[serializedArray->size];
    std::memcpy(receivedData, serializedArray->data, serializedArray->size);
    MutableCharArray_ptr received = boost::make_shared<MutableCharArray>(receivedData,
                                                                         serializedArray->size);
    MutableCharArrayVector_ptr serialized = boost::make_shared<MutableCharArrayVector>();
    serialized->push_back(received);

    Event_ptr eventIn = Event::make(1010, serialized);
    BOOST_REQUIRE_EQUAL(eventIn->isSerialized(0), true);
    IntArray_ptr aOut = eventIn->first<IntArray_ptr>(IntSerializer);
    BOOST_REQUIRE_EQUAL(eventIn->isSerialized(0), false);
    BOOST_REQUIRE_EQUAL(aOut->sharedarr[0], 9);  // The array serializer reduce values by one..
    BOOST_REQUIRE_EQUAL(aOut->sharedarr[1], 19);
    BOOST_REQUIRE_EQUAL(aOut->size, 2);
}


BOOST_AUTO_TEST_SUITE_END()
