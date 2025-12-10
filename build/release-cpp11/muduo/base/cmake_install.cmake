# Install script for directory: /root/install/muduo/muduo/base

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/root/install/build/release-cpp11/lib/libmuduo_base.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("/root/install/build/release-cpp11/muduo/base/CMakeFiles/muduo_base.dir/install-cxx-module-bmi-release.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/muduo/base" TYPE FILE FILES
    "/root/install/muduo/muduo/base/AsyncLogging.h"
    "/root/install/muduo/muduo/base/Atomic.h"
    "/root/install/muduo/muduo/base/BlockingQueue.h"
    "/root/install/muduo/muduo/base/BoundedBlockingQueue.h"
    "/root/install/muduo/muduo/base/Condition.h"
    "/root/install/muduo/muduo/base/CountDownLatch.h"
    "/root/install/muduo/muduo/base/CurrentThread.h"
    "/root/install/muduo/muduo/base/Date.h"
    "/root/install/muduo/muduo/base/Exception.h"
    "/root/install/muduo/muduo/base/FileUtil.h"
    "/root/install/muduo/muduo/base/GzipFile.h"
    "/root/install/muduo/muduo/base/LogFile.h"
    "/root/install/muduo/muduo/base/LogStream.h"
    "/root/install/muduo/muduo/base/Logging.h"
    "/root/install/muduo/muduo/base/Mutex.h"
    "/root/install/muduo/muduo/base/ProcessInfo.h"
    "/root/install/muduo/muduo/base/Singleton.h"
    "/root/install/muduo/muduo/base/StringPiece.h"
    "/root/install/muduo/muduo/base/Thread.h"
    "/root/install/muduo/muduo/base/ThreadLocal.h"
    "/root/install/muduo/muduo/base/ThreadLocalSingleton.h"
    "/root/install/muduo/muduo/base/ThreadPool.h"
    "/root/install/muduo/muduo/base/TimeZone.h"
    "/root/install/muduo/muduo/base/Timestamp.h"
    "/root/install/muduo/muduo/base/Types.h"
    "/root/install/muduo/muduo/base/WeakCallback.h"
    "/root/install/muduo/muduo/base/copyable.h"
    "/root/install/muduo/muduo/base/noncopyable.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/root/install/build/release-cpp11/muduo/base/tests/cmake_install.cmake")

endif()

