# Install script for directory: /home/ctz/work/mRpc/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/home/ctz/work/mRpc/src/build/lib/libmRpc.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/ctz/work/mRpc/src/build/lib/libmRpc.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/home/ctz/work/mRpc/src/build/lib/libmRpc.so"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/ctz/work/mRpc/src/build/lib/libmRpc.so")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/home/ctz/work/mRpc/src/build/lib" TYPE SHARED_LIBRARY FILES "/home/ctz/work/mRpc/src/build/libmRpc.so")
  if(EXISTS "$ENV{DESTDIR}/home/ctz/work/mRpc/src/build/lib/libmRpc.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/ctz/work/mRpc/src/build/lib/libmRpc.so")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/home/ctz/work/mRpc/src/build/lib/libmRpc.so")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/ctz/work/mRpc/src/build/install/include/codeClite.h;/home/ctz/work/mRpc/src/build/install/include/google-inl.h;/home/ctz/work/mRpc/src/build/install/include/rpcChannel.h;/home/ctz/work/mRpc/src/build/install/include/rpcCode.h;/home/ctz/work/mRpc/src/build/install/include/rpcServer.h;/home/ctz/work/mRpc/src/build/install/include/tcpServer.h;/home/ctz/work/mRpc/src/build/install/include/tcpClient.h;/home/ctz/work/mRpc/src/build/install/include/rpc.pb.h")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/home/ctz/work/mRpc/src/build/install/include" TYPE FILE FILES
    "/home/ctz/work/mRpc/src/codeClite.h"
    "/home/ctz/work/mRpc/src/google-inl.h"
    "/home/ctz/work/mRpc/src/rpcChannel.h"
    "/home/ctz/work/mRpc/src/rpcCode.h"
    "/home/ctz/work/mRpc/src/rpcServer.h"
    "/home/ctz/work/mRpc/src/tcpServer.h"
    "/home/ctz/work/mRpc/src/tcpClient.h"
    "/home/ctz/work/mRpc/src/rpc.pb.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/ctz/work/mRpc/src/build/install/src/codeClite.cpp;/home/ctz/work/mRpc/src/build/install/src/rpcChannel.cpp;/home/ctz/work/mRpc/src/build/install/src/rpcCode.cpp;/home/ctz/work/mRpc/src/build/install/src/rpcServer.cpp;/home/ctz/work/mRpc/src/build/install/src/tcpServer.cpp;/home/ctz/work/mRpc/src/build/install/src/tcpClient.cpp;/home/ctz/work/mRpc/src/build/install/src/rpc.pb.cc")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/home/ctz/work/mRpc/src/build/install/src" TYPE FILE FILES
    "/home/ctz/work/mRpc/src/codeClite.cpp"
    "/home/ctz/work/mRpc/src/rpcChannel.cpp"
    "/home/ctz/work/mRpc/src/rpcCode.cpp"
    "/home/ctz/work/mRpc/src/rpcServer.cpp"
    "/home/ctz/work/mRpc/src/tcpServer.cpp"
    "/home/ctz/work/mRpc/src/tcpClient.cpp"
    "/home/ctz/work/mRpc/src/rpc.pb.cc"
    )
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/ctz/work/mRpc/src/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
