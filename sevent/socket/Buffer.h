#pragma once
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include "size.h"


namespace sevent
{
    namespace socket
    {
        class Serialized
        {
            public:
                virtual const char* data() const = 0;
                virtual uint32_t size() const = 0;
        };
        typedef boost::shared_ptr<Serialized> Serialized_ptr;


        class BufferBase
        {
            public:
                virtual Serialized_ptr serialize() = 0;
        };
        typedef boost::shared_ptr<BufferBase> BufferBase_ptr;
        typedef std::vector<BufferBase_ptr> BufferBaseVector;
        typedef boost::shared_ptr<BufferBaseVector> BufferBaseVector_ptr;

        /** Mutable (changable/non-const) buffer. */
        template<typename T, typename SerializeCls>
        class Buffer : public BufferBase
        {
            public:
                static boost::shared_ptr< Buffer<T, SerializeCls> > make(boost::shared_ptr<T> data)
                {
                    return boost::make_shared< Buffer<T, SerializeCls> >(data);
                }
            public:
                Buffer(boost::shared_ptr<T> data) :
                    _data(data) {}
                virtual ~Buffer(){}

                boost::shared_ptr<T> data()
                {
                    return _data;
                }

                Serialized_ptr serialize()
                {
                    return SerializeCls::serialize(_data);
                }

            public:
                static Buffer<T, SerializeCls> deserialize(const char* data, bufsize_t datesize)
                {
                    return SerializeCls::deserialize(data, datesize);
                }

            private:
                boost::shared_ptr<T> _data;
        };



        //class BufferVector {
            //public:
                //typedef std::vector<BufferBase_ptr> Vector_t;
                //typedef boost::shared_ptr<Vector_t> Vector_t_ptr;

            //public:
                //BufferVector()
                //{
                    //vector = boost::make_shared<Vector_t>();
                //}

                //template<typename T, typename SerializeCls>
                //Buffer<T, SerializeCls> at(unsigned index)
                //{
                    //return boost::dynamic_pointer_cast< Buffer<T, SerializeCls> >(vector->at(index));
                //}

                //int size()
                //{
                    //return vector->size();
                //}

            //public:
                //Vector_t_ptr vector;
        //};

    } // namespace socket
} // namespace sevent
