cmake_minimum_required(VERSION 3.8)

if (DEFINED ENV{ROKI_USE_HARDWARE_MOCKS})
  message(INFO " Using hardware mocks")
  set(USE_SCRIPT_MOCKS ON)
  set(USE_PIGPIO_MOCK ON)
else()
  message(INFO " Using real hardware")
  set(USE_SCRIPT_MOCKS OFF)
  set(USE_PIGPIO_MOCK OFF)
endif()

# Make pigpio stop ignore empty pigpio calls
set(PIGPIO_IGNORE ON)

# Use dummy RPC handler 
set(USE_HANDLER_MOCK OFF)

if(USE_SCRIPT_MOCKS)
  add_compile_definitions(SCRIPTS_PREFIX=Mocks/)
else()
  add_compile_definitions(SCRIPTS_PREFIX=Impls/)
endif()

if(USE_HANDLER_MOCK)
  add_compile_definitions(USE_HANDLER_MOCK)
endif()

project(roki-mb-daemon)
include_directories(Inc/)

# Provide pigpio
if(USE_PIGPIO_MOCK)
  if(PIGPIO_IGNORE)
    add_compile_definitions(PIGPIO_IGNORE)
  endif()
  add_library(pigpio Src/PiGpioStub.cpp)
else()
  find_library(PIGPIO_FOUND pigpio)
  add_compile_definitions(PIGPIO_FOUND)
  message(INFO " pigpio found!")
endif()

# Check compile definitions
add_library(CheckDefs Src/CheckDefs.cpp)

# Build Libraries
add_library(Helpers Src/Helpers.cpp)
target_link_libraries(Helpers PUBLIC pthread)

add_library(HwUtils Src/HwUtils.cpp)
target_link_libraries(HwUtils PUBLIC pigpio Helpers CheckDefs)

add_library(FirmwareFSM Src/FirmwareFSM.cpp)
target_link_libraries(FirmwareFSM PUBLIC HwUtils)

add_library(Socket Src/Socket.cpp)
target_link_libraries(Socket PUBLIC Helpers)

add_library(RPC Src/RPC.cpp)
target_link_libraries(RPC PUBLIC Socket)

add_library(RPCDefs Src/RPCDefs.cpp)
target_link_libraries(RPCDefs PUBLIC Helpers)

add_library(Server Src/Server.cpp)
target_link_libraries(Server PUBLIC FirmwareFSM RPC RPCDefs)

add_library(HandlerMock Src/HandlerMock.cpp)

add_library(HandlerImpl Src/HandlerImpl.cpp)
target_link_libraries(HandlerImpl PUBLIC FirmwareFSM)

add_library(Client Src/Client.cpp)
target_link_libraries(Client PUBLIC Server RPCDefs)

add_library(DaemonTools Src/DaemonTools.cpp)
target_link_libraries(DaemonTools PUBLIC Server HandlerMock HandlerImpl CheckDefs)

add_library(CLI Src/CLI.cpp)
target_link_libraries(CLI PUBLIC DaemonTools Client)