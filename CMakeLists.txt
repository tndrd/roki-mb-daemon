cmake_minimum_required(VERSION 3.0)
project(roki-mb-daemon)

add_subdirectory(Deps/googletest)

include_directories(Inc/)

# Provide pigpio
find_library(PIGPIO_FOUND pigpio)

if(NOT PIGPIO_FOUND)
  add_library(pigpio Src/PiGpioStub.cpp)
else()
  add_compile_definitions(PIGPIO_FOUND)
endif()

# Set CLI executable name
set(CLI_EXECUTABLE "mbcli")
add_compile_definitions(CLI_EXECUTABLE=${CLI_EXECUTABLE})

# Build Libraries
add_library(Helpers Src/Helpers.cpp)
target_link_libraries(Helpers PUBLIC pthread)

add_library(HwUtils Src/HwUtils.cpp)
target_link_libraries(HwUtils PUBLIC pigpio Helpers)

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

add_library(Client Src/Client.cpp)
target_link_libraries(Client PUBLIC Server RPCDefs)

add_library(DaemonTools Src/DaemonTools.cpp)
target_link_libraries(DaemonTools PUBLIC Server)

add_library(CLI Src/CLI.cpp)
target_link_libraries(CLI PUBLIC DaemonTools Client)

# Build Executables
add_executable(${CLI_EXECUTABLE} Executables/CLIMain.cpp)
target_link_libraries(${CLI_EXECUTABLE} PUBLIC CLI)