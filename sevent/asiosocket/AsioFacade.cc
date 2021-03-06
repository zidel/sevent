#include "AsioFacade.h"
#include "AsioSession.h"
#include "AsioListener.h"
#include <iostream>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

namespace sevent
{
    namespace asiosocket
    {
        AsioFacade::AsioFacade()
        {
            _service = AsioService::make();
            _sessionRegistry = socket::SessionRegistry::make();
            _connector = socket::Connector_ptr(new AsioConnector(
                        _service, _sessionRegistry));
        }
        AsioFacade::~AsioFacade()
        {
        }

        AsioFacade_ptr AsioFacade::make()
        {
            return AsioFacade_ptr(new AsioFacade());
        }

        void AsioFacade::setWorkerThreads(unsigned numberOfWorkerThreads,
                                                allEventsHandler_t allEventsHandler)
        {
            setWorkerThreads(numberOfWorkerThreads, defaultWorkerThreadHandler, allEventsHandler);
        }

        void AsioFacade::setWorkerThreads(unsigned numberOfWorkerThreads,
                                                workerThread_t workerThreadHandler,
                                                allEventsHandler_t allEventsHandler)
        {
            _allEventsHandler = allEventsHandler;
            _sessionRegistry->setAllEventsHandler(
                boost::bind(allEventsHandler, shared_from_this(), _1, _2));
            for (unsigned x = 0; x < numberOfWorkerThreads; ++x)
            {
                _worker_threads.create_thread(
                    boost::bind(workerThreadHandler, shared_from_this()));
            }
        }

        socket::Listener_ptr AsioFacade::listen(socket::Address_ptr address)
        {
            if(!_allEventsHandler)
            {
                std::runtime_error("Can not create a listener without calling setWorkerThreads first.");
            }
            socket::Listener_ptr listener = boost::make_shared<AsioListener>(_service, _sessionRegistry);
            listener->listen(address);
            saveListener(*address, listener);
            return listener;
        }

        socket::Session_ptr AsioFacade::connect(socket::Address_ptr address)
        {
            return _connector->connect(address);
        }

        socket::Service_ptr AsioFacade::service()
        {
            return _service;
        }

        socket::SessionRegistry_ptr AsioFacade::sessionRegistry()
        {
            return _sessionRegistry;
        }

        void AsioFacade::joinAllWorkerThreads()
        {
            _worker_threads.join_all();
        }

        void AsioFacade::sendEvent(socket::Session_ptr session,
                                   event::Event_ptr event)
        {
            if(isLocalSession(session))
            {
                invokeAllEventsHandler(session, event);
            } else
            {
                session->sendEvent(event);
            }
        }

        void AsioFacade::invokeAllEventsHandler(socket::Session_ptr session,
                                                event::Event_ptr event)
        {
            _allEventsHandler(shared_from_this(), session, event);
        }

        void AsioFacade::defaultWorkerThreadHandler(socket::Facade_ptr facade)
        {
            try
            {
                facade->service()->run();
            }
            catch (boost::exception& e)
            {
                std::cerr << "[" << boost::this_thread::get_id()
                          << "] Exception: " << boost::diagnostic_information(e)
                          << std::endl;
            }
            catch (std::exception& e)
            {
                std::cerr << "[" << boost::this_thread::get_id()
                          << "] Exception: " << e.what() << std::endl;
            }
            facade->service()->stop(); // As soon as one worker fails, we abort all of them (good for debugging)
        }
    }
}
