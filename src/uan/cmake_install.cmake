# Install script for directory: /home/renan/ns-allinone-3.42/ns-3.42/src/uan

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
    set(CMAKE_INSTALL_CONFIG_NAME "default")
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
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.42-uan-default.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.42-uan-default.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.42-uan-default.so"
         RPATH "/usr/local/lib:$ORIGIN/:$ORIGIN/../lib:/usr/local/lib64:$ORIGIN/:$ORIGIN/../lib64")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/renan/ns-allinone-3.42/ns-3.42/build/lib/libns3.42-uan-default.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.42-uan-default.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.42-uan-default.so")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.42-uan-default.so"
         OLD_RPATH "/home/renan/ns-allinone-3.42/ns-3.42/build/lib:::::::::::::::::::::::::::::::::::"
         NEW_RPATH "/usr/local/lib:$ORIGIN/:$ORIGIN/../lib:/usr/local/lib64:$ORIGIN/:$ORIGIN/../lib64")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.42-uan-default.so")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ns3" TYPE FILE FILES
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/helper/acoustic-modem-energy-model-helper.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/helper/uan-helper.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/model/acoustic-modem-energy-model.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/model/uan-channel.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/model/uan-header-common.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/model/uan-header-rc.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/model/uan-mac-aloha.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/model/uan-mac-cw.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/model/uan-mac-rc-gw.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/model/uan-mac-rc.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/model/uan-mac.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/model/uan-net-device.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/model/uan-noise-model-default.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/model/uan-noise-model.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/model/uan-phy-dual.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/model/uan-phy-gen.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/model/uan-phy.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/model/uan-prop-model-ideal.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/model/uan-prop-model-thorp.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/model/uan-prop-model.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/model/uan-transducer-hd.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/model/uan-transducer.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/src/uan/model/uan-tx-mode.h"
    "/home/renan/ns-allinone-3.42/ns-3.42/build/include/ns3/uan-module.h"
    )
endif()

