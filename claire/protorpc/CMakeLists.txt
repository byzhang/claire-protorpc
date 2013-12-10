#generate assets file
set(RPC_ASSETS
  ${CMAKE_CURRENT_SOURCE_DIR}/assets/forms.html
  ${CMAKE_CURRENT_SOURCE_DIR}/assets/forms.js
  ${CMAKE_CURRENT_SOURCE_DIR}/assets/methods.html
)

add_custom_command(OUTPUT RpcAssets.h RpcAssets.cc
  COMMAND gen-assets
  ARGS RpcAssets ${RPC_ASSETS}
  COMMAND cp
  ARGS RpcAssets.h RpcAssets.cc ${CMAKE_CURRENT_SOURCE_DIR}
  MAIN_DEPENDENCY ${RPC_ASSETS}
  VERBATIM )

#generate protobuf files
add_executable(protoc-gen-rpc generator/protoc-gen-rpc.cc)
target_link_libraries(protoc-gen-rpc ${PROTOBUF_LIBRARY} ${PROTOBUF_PROTOC_LIBRARY} pthread)

set(PROTO_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(PROTO_SRC ${PROTO_PATH}/builtin_service.proto ${PROTO_PATH}/rpcmessage.proto)
add_custom_command(OUTPUT builtin_service.pb.cc builtin_service.pb.h rpcmessage.pb.cc rpcmessage.pb.h
  COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
  ARGS --plugin=${PROJECT_BINARY_DIR}/bin/protoc-gen-rpc --rpc_out .
       --proto_path=${PROTO_PATH} ${PROTO_SRC} -I${PROJECT_SOURCE_DIR}
       -I${PROTOBUF_INCLUDE_DIR}  --descriptor_set_out=builtin_service.desc
  COMMAND sed
  ARGS -i "s/#include <google\\/protobuf\\/service.h>/#include <claire\\/protorpc\\/service.h>/" builtin_service.pb.h
  COMMAND sed
  ARGS -i "s/#include .rpcmessage.pb.h./#include <claire\\/protorpc\\/rpcmessage.pb.h>/" builtin_service.pb.h
  COMMAND sed
  ARGS -i "/protoc_insertion_point.includes./i#include <boost/shared_ptr.hpp>" rpcmessage.pb.h
  COMMAND cp
  ARGS rpcmessage.pb.h rpcmessage.pb.cc ${CMAKE_CURRENT_SOURCE_DIR}
  MAIN_DEPENDENCY ${PROTO_SRC}
  DEPENDS protoc-gen-rpc
  VERBATIM )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=conversion -Wno-error=shadow -Wno-extra")
include_directories(${PROJECT_BINARY_DIR})

#compile protorpc project
set(PROTORPC_SRCS
    rpcmessage.pb.cc
    builtin_service.pb.cc
    BuiltinService.cc
    RpcCodec.cc
    RpcChannel.cc
    RpcServer.cc
    RpcController.cc
    RpcUtil.cc
    RpcAssets.cc)

add_library(claire_protorpc ${PROTORPC_SRCS})
target_link_libraries(claire_protorpc claire_netty claire_common
    ${PROTOBUF_LIBRARY} snappy
    pthread z)
add_dependencies(claire_protorpc protoc-gen-rpc)

install(TARGETS protoc-gen-rpc DESTINATION bin)
install(TARGETS claire_protorpc DESTINATION lib)

#install headers
set(HEADERS
  service.h
  BuiltinService.h
  RpcServer.h
  RpcChannel.h
  rpcmessage.proto
  builtin_service.proto
  ${PROJECT_BINARY_DIR}/claire/protorpc/rpcmessage.pb.h
  ${PROJECT_BINARY_DIR}/claire/protorpc/builtin_service.pb.h
  )
install(FILES ${HEADERS} DESTINATION include/claire/protorpc)
