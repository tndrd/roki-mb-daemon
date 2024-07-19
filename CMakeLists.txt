cmake_minimum_required(VERSION 3.0)

add_compile_options("-g")

# Set CLI executable name
set(CLI_EXECUTABLE mbctl)
set(MY_INSTALL_PREFIX roki/mb)
set(SCRIPTS_PATH /usr/lib/${MY_INSTALL_PREFIX}/Scripts)

add_compile_definitions(CLI_EXECUTABLE=${CLI_EXECUTABLE})

set(PIGPIO_IGNORE ON)
set(USE_SCRIPT_MOCKS OFF)
set(USE_HANDLER_MOCK OFF)

if(USE_SCRIPT_MOCKS)
  set(SCRIPTS_DIR ${SCRIPTS_PATH}/Mocks)
else()
  set(SCRIPTS_DIR ${SCRIPTS_PATH}/Impls)
endif()

add_compile_definitions(SCRIPTS_DIR=${SCRIPTS_DIR})

if(USE_HANDLER_MOCK)
  add_compile_definitions(USE_HANDLER_MOCK)
endif()

if(USE_SCRIPT_MOCKS)
  add_compile_definitions(USE_SCRIPT_MOCKS)
endif()


project(roki-mb-daemon)
include_directories(Inc/)

# Provide pigpio
find_library(PIGPIO_FOUND pigpio)

if(NOT PIGPIO_FOUND)
  if(PIGPIO_IGNORE)
    add_compile_definitions(PIGPIO_IGNORE)
  endif()
  add_library(pigpio Src/PiGpioStub.cpp)
else()
  add_compile_definitions(PIGPIO_FOUND)
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

# Build Executables
add_executable(${CLI_EXECUTABLE} Executables/CLIMain.cpp)
target_link_libraries(${CLI_EXECUTABLE} PUBLIC CLI)

# Installation rules
install(TARGETS ${CLI_EXECUTABLE} RUNTIME DESTINATION /usr/bin)

set(CMAKE_INSTALL_PREFIX /usr/lib/${MY_INSTALL_PREFIX})
install(TARGETS Client)
install(DIRECTORY Inc/ DESTINATION /usr/include/${MY_INSTALL_PREFIX})
install(DIRECTORY Scripts DESTINATION /usr/lib/${MY_INSTALL_PREFIX})

# install(TARGETS Client )
