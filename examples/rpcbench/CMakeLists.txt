PROTOBUF_GENERATE_RPC(PROTO_SRCS echo.proto)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=conversion -Wno-extra -Wno-shadow")
include_directories(${PROJECT_BINARY_DIR})

add_library(echo_proto ${PROTO_SRCS})

add_executable(echo_client client.cc)
target_link_libraries(echo_client echo_proto
    claire_protorpc claire_netty claire_common
    gflags tcmalloc
    boost_regex cares)

add_executable(echo_server server.cc)
target_link_libraries(echo_server echo_proto
    claire_protorpc claire_netty claire_common
    gflags tcmalloc profiler ctemplate
    boost_regex cares)

