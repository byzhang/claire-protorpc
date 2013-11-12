# claire-protorpc Introduction

claire-protorpc is an implemetion of Google Protocol Buffers, based on claire-netty and claire-common library.claire-protorpc only support C++ now, required run on x86-64 linux only(kernel > 2.6.32, gcc > 4.4.6).

claire-protorpc generate protobuf .proto code use self-implemented plugin protoc-rpc-gen, which can integrate more features to RPC implmentios.

## Implemention Features ##

 - Http Transport
 - Compress Transport
 - NonBlock at all(even dns)
 - Resource manged by smart pointer: Used never need care about memory leak
 - Multi-thread enabled: Thread safe too!
 - Modern C++ Style: boost::bind/function, std::move/forward, boost::noncopyable, every where!

## Server Features ##

 - Service Reigstration: Servide support any service genrated by claire-protorpc defined protoc plugin protoc-rpc-gen.
 - /protorpc: User can send json message to /json path, and server can convert it to protobuf message automatically
 - /form: Server support export service method and method request/response definition to use in form, easy to debug RPC service. 
 - /flags: Server support export internal gflags item to used through /flags path. And user can change the flags value through /flags path use Post method.
 - /status: Server support export internal status, like Startime, Running Thread, etc.
 - /pprof: Server support online CPU/Memory profilling throw /pprof/profile and /pprof/heap, more info can find [here][1]

## Client Features ## 

 - Load Balancing: currently support random and rr policy
 - Address Resolver: currently support static address and dns.
 - Retry
 - Heartbeat


## Dependency ##

  - boost
  - gflags
  - protobuf
  - ctemplate
  - gperftools
  - claire-common
  - claire-netty
  - c-ares(optional)

  [1]: https://code.google.com/p/gperftools/source/browse/doc/pprof_remote_servers.html
