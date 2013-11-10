#include <examples/rpcbench/echo.pb.h>

#include <stdio.h>

#include <boost/random.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <claire/common/threading/CountDownLatch.h>
#include <claire/common/logging/Logging.h>
#include <claire/common/eventloop/EventLoop.h>
#include <claire/common/eventloop/EventLoopThread.h>
#include <claire/common/eventloop/EventLoopThreadPool.h>
#include <claire/netty/InetAddress.h>
#include <claire/netty/http/HttpClient.h>
#include <claire/common/metrics/Counter.h>
#include <claire/common/metrics/CounterProvider.h>

#include <claire/protorpc/RpcChannel.h>

using namespace claire;

static const int kRequests = 50000;

class RpcClient : boost::noncopyable
{
public:

    RpcClient(EventLoop* loop,
              CountDownLatch* allFinished,
              int id, int max)
      : channel_(loop),
        stub_(&channel_),
        allFinished_(allFinished),
        count_(0),
        finished_(false),
        id_(id),
        max_(max),
        total_request("total_request"),
        total_response("total_response")
    {
    }

    void connect(const InetAddress& addr)
    {
        channel_.Connect(addr);
    }

    void SendRequest()
    {
        RpcControllerPtr controller(new RpcController());
        Send(controller);
    }

private:
    void Send(RpcControllerPtr& controller)
    {
        controller->Reset();
        echo::EchoRequest request;
        request.set_str("0123456789ABCDEF");
        stub_.Echo(controller, request, boost::bind(&RpcClient::replied, this, _1, _2));
        total_request.Increment();
    }

    void replied(RpcControllerPtr& controller, const boost::shared_ptr<echo::EchoResponse>& resp)
    {
        if (controller->Failed())
        {
            LOG(ERROR) << controller->ErrorText();
        }

        ++count_;
        total_response.Increment();
        if (count_ < max_)
        {
            Send(controller);
        }
        else if (!finished_)
        {
            finished_ = true;
            allFinished_->CountDown();
            LOG(INFO) << "RpcClient#" << id_ << " finished, allFinished left " << allFinished_->get_count();
        }
    }

    RpcChannel channel_;
    echo::EchoService::Stub stub_;
    CountDownLatch* allFinished_;
    int count_;
    bool finished_;
    int id_;
    int max_;
    Counter total_request;
    Counter total_response;
};

int main(int argc, char* argv[])
{
    ::google::ParseCommandLineFlags(&argc, &argv, true);
    if (argc > 1)
    {
        int nClients = argc > 2 ? atoi(argv[2]) : 1;
        int nThreads = argc > 3 ? atoi(argv[3]) : 1;
        int nPipeLine = argc > 4 ? atoi(argv[4]) : 1;

        CountDownLatch allFinished(nClients);

        EventLoop loop;
        EventLoopThreadPool pool(&loop);
        pool.SetThreadNum(nThreads);
        pool.Start();

        boost::mt19937 rng;
        boost::uniform_int<> six(1, kRequests);
        boost::variate_generator<boost::mt19937&, boost::uniform_int<> > die(rng, six);

        InetAddress serverAddr(argv[1], 8080);

        boost::ptr_vector<RpcClient> clients;
        for (int i = 0; i < nClients; ++i)
        {
            clients.push_back(new RpcClient(pool.GetNextLoop(), &allFinished, i, die()));
            clients.back().connect(serverAddr);
        }
        sleep(1);

        LOG(INFO) << "all connected";
        LOG_WARNING << "start";

        Timestamp start(Timestamp::Now());
        for (int i = 0; i < nClients; ++i)
        {
            for (int j = 0; j < nPipeLine; j++)
                clients[i].SendRequest();
        }
        allFinished.Wait();
        Timestamp end(Timestamp::Now());
        LOG(INFO) << "all finished";

        double seconds = static_cast<double>(TimeDifference(end, start))/1000000;
        printf("%f seconds\n", seconds);
        printf("%.1f calls per second\n", static_cast<double>(static_cast<int32_t>(CounterProvider::instance()->GetCounterValue("c:total_response")) / seconds));
        LOG(INFO) << "total request: " << CounterProvider::instance()->GetCounterValue("c:total_request") 
                  << ", total response: " << CounterProvider::instance()->GetCounterValue("c:total_response");
        exit(0);
    }
    else
    {
        printf("Usage: %s host_ip num_clients [num_threads [piplines]]\n", argv[0]);
    }
}

