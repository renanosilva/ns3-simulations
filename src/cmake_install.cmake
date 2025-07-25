# Install script for directory: /home/renan/ns-allinone-3.42/ns-3.42/src

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

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/antenna/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/aodv/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/applications/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/bridge/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/brite/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/buildings/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/click/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/config-store/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/core/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/csma/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/csma-layout/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/dsdv/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/dsr/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/energy/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/fd-net-device/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/flow-monitor/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/internet/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/internet-apps/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/lr-wpan/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/lte/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/mesh/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/mobility/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/netanim/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/network/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/nix-vector-routing/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/olsr/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/openflow/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/point-to-point/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/point-to-point-layout/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/propagation/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/sixlowpan/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/spectrum/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/stats/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/tap-bridge/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/topology-read/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/traffic-control/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/uan/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/virtual-net-device/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/wifi/cmake_install.cmake")
  include("/home/renan/ns-allinone-3.42/ns-3.42/src/wimax/cmake_install.cmake")

endif()

