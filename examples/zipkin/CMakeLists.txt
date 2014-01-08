PROTOBUF_GENERATE_RPC(PROTO_SRCS sort.proto)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=conversion -Wno-extra -Wno-shadow")
include_directories(${PROJECT_BINARY_DIR})

add_library(sort_proto ${PROTO_SRCS})

add_executable(sort_client client.cc)
target_link_libraries(sort_client sort_proto
    claire_protorpc claire_netty claire_zipkin claire_common
    gflags tcmalloc thrift
    boost_regex pthread)

add_executable(sort_transfer transfer.cc)
target_link_libraries(sort_transfer sort_proto
    claire_protorpc claire_netty claire_zipkin claire_common
    gflags tcmalloc thrift ctemplate profiler
    boost_regex pthread)

add_executable(sort_server server.cc)
target_link_libraries(sort_server sort_proto
    claire_protorpc claire_netty claire_zipkin claire_common
    gflags tcmalloc profiler ctemplate thrift
    boost_regex pthread)

