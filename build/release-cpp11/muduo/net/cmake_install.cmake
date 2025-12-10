# Install script for directory: /root/install/muduo/muduo/net

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/root/install/build/release-install-cpp11")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "release")
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

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/root/install/build/release-cpp11/lib/libmuduo_net.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("/root/install/build/release-cpp11/muduo/net/CMakeFiles/muduo_net.dir/install-cxx-module-bmi-release.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/muduo/net" TYPE FILE FILES
    "/root/install/muduo/muduo/net/Buffer.h"
    "/root/install/muduo/muduo/net/Callbacks.h"
    "/root/install/muduo/muduo/net/Channel.h"
    "/root/install/muduo/muduo/net/Endian.h"
    "/root/install/muduo/muduo/net/EventLoop.h"
    "/root/install/muduo/muduo/net/EventLoopThread.h"
    "/root/install/muduo/muduo/net/EventLoopThreadPool.h"
    "/root/install/muduo/muduo/net/InetAddress.h"
    "/root/install/muduo/muduo/net/TcpClient.h"
    "/root/install/muduo/muduo/net/TcpConnection.h"
    "/root/install/muduo/muduo/net/TcpServer.h"
    "/root/install/muduo/muduo/net/TimerId.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/root/install/build/release-cpp11/muduo/net/http/cmake_install.cmake")
  include("/root/install/build/release-cpp11/muduo/net/inspect/cmake_install.cmake")
  include("/root/install/build/release-cpp11/muduo/net/tests/cmake_install.cmake")
  include("/root/install/build/release-cpp11/muduo/net/protobuf/cmake_install.cmake")
  include("/root/install/build/release-cpp11/muduo/net/protorpc/cmake_install.cmake")

endif()

