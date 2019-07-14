cmake_minimum_required(VERSION 3.10)

#CMAKE_CURRENT_SOURCE_DIR---路径指向当前正在处理的源目录---CMakeLists.txt
#所在目录,这是一个指向源目录的完全路径，当前正在被cmake处理。
add_custom_command(OUTPUT rpc.pb.cc rpc.pb.h
  COMMAND protoc ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/rpc.proto -I${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS rpc.proto 
  VERBATIM )

add_custom_target(rpc DEPENDS rpc.pb.cc rpc.pb.h)

#打印当前PROJECT_SOURCE_DIR
MESSAGE(STATUS "this PROJECT_SOURCE_DIR is ${PROJECT_SOURCE_DIR}")
MESSAGE(STATUS "this PROJECT_BINARY_DIR is ${PROJECT_BINARY_DIR}")
INCLUDE_DIRECTORIES(../include)
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/src)

set(SOURCE_FILES 
  codeClite.cpp
  rpcChannel.cpp
  rpcCode.cpp
  rpcServer.cpp
  tcpServer.cpp
  tcpClient.cpp
  rpc.pb.cc)

ADD_LIBRARY(mRpc SHARED ${SOURCE_FILES})
#两个targets有依赖关系（通过target_link_libraries解决）并且依赖库也是通过
#编译源码产生的
#这时候一句add_dependencies可以在直接编译上层target时，自动检查下层依赖库
#是否已经生成
#没有的话先编译下层依赖库，然后再编译上层target，最后link depend target。
add_dependencies(mRpc rpc)
#依赖的库文件
TARGET_LINK_LIBRARIES(mRpc glog pthread event)

install(TARGETS mRpc DESTINATION ${PROJECT_BINARY_DIR}/lib/)

SET(HEADERS
    codeClite.h
    google-inl.h
    rpcChannel.h
    rpcCode.h
    rpcServer.h
    tcpServer.h
    tcpClient.h
    rpc.pb.h
    )

install(FILES ${HEADERS} DESTINATION ${PROJECT_BINARY_DIR}/install/include)
install(FILES ${SOURCE_FILES} DESTINATION ${PROJECT_BINARY_DIR}/install/src)
