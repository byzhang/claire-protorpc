#include <examples/zipkin/sort.pb.h>

#include <claire/common/tracing/Tracing.h>
#include <claire/zipkin/ZipkinTracer.h>
#include <claire/common/logging/Logging.h>
#include <examples/zipkin/client.h>

using namespace claire;

EventLoop* g_loop = NULL;
void OnResult(const std::vector<int32_t>& v)
{
    for (size_t i = 0; i < v.size(); i++)
    {
        LOG(DEBUG) << "v[" << i << "] " << v[i];
    }
    g_loop->quit();
}

int main(int argc, char* argv[])
{
    ::google::ParseCommandLineFlags(&argc, &argv, true);
    InitClaireLogging(argv[0]);

    ZipkinTracer tracer(InetAddress(9410));
    InstallClaireTracer(&tracer);

    EventLoop loop;
    g_loop = &loop;
    InetAddress server_address(argv[1], 8081);

    SortClient client(&loop, server_address);
    std::vector<int32_t> v;
    v.push_back(10);
    v.push_back(1);
    v.push_back(11);
    v.push_back(2);
    v.push_back(100);
    v.push_back(8);
    v.push_back(12);
    v.push_back(9);
    client.SendRequest(v, boost::bind(&OnResult, _1));
    loop.loop();
    return 0;
}

