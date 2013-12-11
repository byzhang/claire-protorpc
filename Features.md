# Features

claire-protorpc is an implemetion of Google Protocol Buffers based RPC library, based on claire-netty and claire-common library. claire-protorpc only support C++ now, required run on x86-64 linux only(kernel > 2.6.32, gcc > 4.4.6).

claire-protorpc generate protobuf .proto code use self-implemented plugin protoc-rpc-gen, which can integrate more features to RPC implmentios.

## Common Features ##

 - Moden C++, use C++ TR1 features, such as bind/function. etc.
 - Reactor model, complete non-blocking, event dns resolve.
 - Multicore ready, thread safe library.
 - Automatic resouce management, use almost no need care about memory release, even in multithread enviroment.

## Server Features ##

 -  claire-protorpc defined protoc plugin protoc-rpc-gen itself.
 -  RpcServer is built on HttpServer, with better penetrativity.
 -  Support translate json message to protobuf message automatically, easy for debug and test.
 -  Support export service method and method request/response definition to use in form, easy to debug RPC service. 
 -  Support export internal gflags item to used through /flags path. And user can change the flags value through /flags path use Post method.
 -  Support online CPU/Memory profilling throw /pprof/profile and /pprof/heap.
 -  Support support output internal statistic to use  through /couters and /histograms, which is graphcal!
 -  Support compress/uncompress message.

## Client Features ##

 - Support random load balancer, more on the way!
 - Support static address and dns.
 - Use heartbeat message detect the healthy of connection.
 - Support compress/uncompress message.
 - Support request with timeout through method/service options.

