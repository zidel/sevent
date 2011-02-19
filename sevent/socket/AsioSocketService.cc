#include "AsioSocketService.h"

namespace sevent
{
namespace socket
{

AsioSocketService::AsioSocketService() :
    _work(_io_service)
{
}

AsioSocketService::~AsioSocketService()
{
}

AsioSocketService_ptr AsioSocketService::make()
{
    return AsioSocketService_ptr(new AsioSocketService());
}

void AsioSocketService::run()
{
    _io_service.run();
}

void AsioSocketService::stop()
{
    _io_service.stop();
}

} // namespace socket
} // namespace sevent