#include "AsioSession.h"
#include <iostream>
#include <stdint.h>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_array.hpp>
#include <boost/foreach.hpp>
#include <arpa/inet.h>
#include "MutableBuffer.h"

namespace sevent
{
    namespace asiosocket
    {

        AsioSession::AsioSession(socket_ptr sock) :
            _sock(sock)
        {
            setAllEventsHandler( defaultAllEventsHandler);
        }

        AsioSession::~AsioSession()
        {
        }

        AsioSession_ptr AsioSession::make(socket_ptr sock)
        {
            return AsioSession_ptr(new AsioSession(sock));
        }


        void AsioSession::sendHeader(unsigned eventid, int numElements)
        {
            uint32_t eventIdNetworkOrder = htonl(eventid);
            uint32_t numElementsNetworkOrder = htonl(numElements);
            boost::asio::write(*_sock,
                               boost::asio::buffer(&eventIdNetworkOrder,
                                                  sizeof(uint32_t)),
                               boost::asio::transfer_all());
            boost::asio::write(*_sock,
                               boost::asio::buffer(&numElementsNetworkOrder,
                                                  sizeof(uint32_t)),
                               boost::asio::transfer_all());
        }

        void AsioSession::sendData(const socket::ConstBuffer& data)
        {
            uint32_t sizeNetworkOrder = htonl(data.size());
            const char* s = (const char*) data.data();
            boost::asio::write(*_sock,
                               boost::asio::buffer(&sizeNetworkOrder,
                                                   sizeof(uint32_t)),
                               boost::asio::transfer_all());
            boost::asio::write(*_sock,
                               boost::asio::buffer(data.data(), data.size()),
                               boost::asio::transfer_all());
        }

        void AsioSession::sendEvent(unsigned eventid, const socket::ConstBuffer& data)
        {
            boost::lock_guard<boost::mutex> lock(_sendLock);
            sendHeader(eventid, 1);
            sendData(data);
        }

        void AsioSession::sendEvent(unsigned eventid, const socket::ConstBufferVector& dataBufs)
        {
            boost::lock_guard<boost::mutex> lock(_sendLock);
            sendHeader(eventid, dataBufs.size());
            BOOST_FOREACH(const socket::ConstBuffer& data, dataBufs)
            {
                sendData(data);
            }
        }

        void AsioSession::receiveEvents()
        {
            _receiveLock.lock();
            boost::asio::async_read(*_sock,
                                    boost::asio::buffer(_headerBuf),
                                    boost::asio::transfer_all(),
                                    boost::bind(&AsioSession::onHeaderReceived,
                                                this, _1, _2));
        }


        socket::MutableBuffer_ptr AsioSession::receiveData()
        {
            boost::array<uint32_t, 1> dataSizeBuf;
            int bytes_read = boost::asio::read(*_sock,
                                               boost::asio::buffer(dataSizeBuf),
                                               boost::asio::transfer_all());
            assert(bytes_read == sizeof(uint32_t));
            uint32_t dataSize = ntohl(dataSizeBuf[0]);

            boost::shared_array<char> data = boost::shared_array<char>(new char[dataSize]);
            boost::asio::read(*_sock,
                              boost::asio::buffer(data.get(), dataSize),
                              boost::asio::transfer_all());

            socket::MutableBuffer_ptr mb;
            mb = boost::make_shared<socket::MutableBuffer>(data, dataSize);
            return mb;
        }

        socket::MutableBufferVector_ptr AsioSession::receiveAllData(unsigned numElements)
        {
            socket::MutableBufferVector_ptr dataBufs = boost::make_shared<socket::MutableBufferVector>();
            for(int i = 0; i < numElements; i++)
            {
                socket::MutableBuffer_ptr mb = receiveData();
                dataBufs->push_back(mb);
            }
            return dataBufs;
        }

        void AsioSession::onHeaderReceived(const boost::system::error_code & error,
                                           std::size_t byte_transferred)
        {
            if(!handleTransferErrors(error,
                                     byte_transferred,
                                     sizeof(uint32_t)*2,
                                     "byte_transferred != sizeof(uint32_t)*2"))
            {
                return;
            }
            uint32_t eventid = ntohl(_headerBuf[0]);
            uint32_t numElements = ntohl(_headerBuf[1]);

            { // Make the dataBufs go out of scope as fast as possible
                socket::MutableBufferVector_ptr dataBufs = receiveAllData(numElements);
                socket::ReceiveEvent event(eventid, dataBufs);
                _allEventsHandler(shared_from_this(), event);
            }

            _receiveLock.unlock();
            receiveEvents();
        }

        bool AsioSession::handleTransferErrors(const boost::system::error_code& error,
                                               uint32_t bytesTransferred,
                                               uint32_t expectedBytesTransferred,
                                               const char* bytesTransferredErrmsg)
        {
            if (error == boost::asio::error::eof)
            {
                _disconnectHandler(shared_from_this());
                return false;
            }
            else if (error)
            {
                throw boost::system::system_error(error);
            }
            else if(bytesTransferred != expectedBytesTransferred)
            {
                throw std::runtime_error(bytesTransferredErrmsg); // This is a bug, because transfer_all() should make this impossible.
            }
            return true;
        }


        socket::Address_ptr AsioSession::getRemoteEndpointAddress()
        {
            std::string host = _sock->remote_endpoint().address().to_string();
            unsigned short port = _sock->remote_endpoint().port();
            return socket::Address::make(host, port);
        }

        socket::Address_ptr AsioSession::getLocalEndpointAddress()
        {
            std::string host = _sock->local_endpoint().address().to_string();
            unsigned short port = _sock->local_endpoint().port();
            return socket::Address::make(host, port);
        }

    } // namespace asiosocket
} // namespace sevent
