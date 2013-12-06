#include <examples/rpcbench/echo.pb.h>

#include <boost/bind.hpp>

#include <claire/protorpc/RpcServer.h>
#include <claire/common/events/EventLoop.h>
#include <claire/common/logging/Logging.h>
#include <claire/netty/InetAddress.h>

using namespace claire;

DEFINE_int32(num_threads, 4, "num of RpcServer threads");

namespace echo
{

class EchoServiceImpl : public EchoService
{
public:
    virtual void Echo(RpcControllerPtr& controller,
                      const ::echo::EchoRequestPtr& request,
                      const ::echo::EchoResponse* responseProtoType,
                      const RpcDoneCallback& done)
    {
        EchoResponse response;
        response.set_str(request->str());
        done(controller, &response);
    }
};

}

int main(int argc, char* argv[])
{
    ::google::ParseCommandLineFlags(&argc, &argv, true);
    InitClaireLogging(argv[0]);

    EventLoop loop;
    InetAddress listenAddr(8080);
    echo::EchoServiceImpl impl;
    RpcServer server(&loop, listenAddr);
    server.set_num_threads(FLAGS_num_threads);
    server.RegisterService(&impl);
    server.Start();
    loop.loop();
}

