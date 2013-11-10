PROTOBUF_GENERATE_RPC(PROTO_SRCS echo.proto)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=conversion -Wno-extra -Wno-shadow")
include_directories(${PROJECT_BINARY_DIR})

add_library(echo_proto ${PROTO_SRCS})

add_executable(protorpc_echo_client client.cc)
target_link_libraries(protorpc_echo_client echo_proto claire_protorpc
    claire_resolver claire_loadbalancer
    claire_http claire_netty claire_common boost_system gflags boost_regex)
add_dependencies(protorpc_echo_client claire_resolver claire_loadbalancer)

add_executable(protorpc_echo_server server.cc)
target_link_libraries(protorpc_echo_server echo_proto claire_protorpc
    claire_resolver claire_loadbalancer
    claire_http claire_netty claire_common boost_system gflags
    boost_regex)

