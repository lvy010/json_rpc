# Install script for directory: /root/install/muduo/examples

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

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/root/install/build/release-cpp11/examples/ace/ttcp/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/asio/chat/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/asio/tutorial/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/fastcgi/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/filetransfer/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/hub/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/idleconnection/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/maxconnection/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/memcached/client/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/memcached/server/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/multiplexer/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/netty/discard/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/netty/echo/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/netty/uptime/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/pingpong/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/roundtrip/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/shorturl/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/simple/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/socks4a/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/sudoku/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/twisted/finger/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/wordcount/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/zeromq/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/ace/logging/cmake_install.cmake")
  include("/root/install/build/release-cpp11/examples/protobuf/cmake_install.cmake")

endif()

