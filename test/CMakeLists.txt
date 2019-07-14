
add_custom_command(OUTPUT rpc.pb.cc evrpc.pb.h
  COMMAND protoc ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/rpc.proto -I${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS rpc.proto
  VERBATIM )
add_custom_target(server_proto DEPENDS rpc.pb.cc rpc.pb.h)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/test)
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/src)
LINK_DIRECTORIES(${LIBRARY_OUTPUT_DIRECTORY})


#ADD_EXECUTABLE(tcpclient_test tcpclient_test.cc)
#ADD_EXECUTABLE(tcpserver_test tcpserver_test.cc)

ADD_EXECUTABLE(server server.cpp rpc.pb.cc)
ADD_EXECUTABLE(client client.cpp rpc.pb.cc)

add_dependencies(server server_proto)
add_dependencies(client server_proto)

#TARGET_LINK_LIBRARIES(tcpclient_test evrpc glog pthread event)
#TARGET_LINK_LIBRARIES(tcpserver_test evrpc glog pthread event)
TARGET_LINK_LIBRARIES(server mRpc z protobuf glog pthread event)
TARGET_LINK_LIBRARIES(client mRpc z protobuf glog pthread event)





