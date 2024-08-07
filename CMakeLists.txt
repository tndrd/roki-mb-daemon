cmake_minimum_required(VERSION 3.8)

function(ASSERT_VAR VARNAME)
  if (NOT DEFINED ${VARNAME})
    message(FATAL_ERROR ${VARNAME} " var is not set")
  endif()
endfunction()

ASSERT_VAR(MODULE_NAME)
ASSERT_VAR(DAEMON_CLI_EXECUTABLE)
ASSERT_VAR(DAEMON_PORT_ENV)
ASSERT_VAR(DAEMON_LOGFILE_ENV)
ASSERT_VAR(DAEMON_BACKLOG_ENV)
ASSERT_VAR(DAEMON_CONN_ATTEMPT_COUNT_ENV)
ASSERT_VAR(DAEMON_CONN_ATTEMPT_PERIOD_ENV)

if (DEFINED ENV{ROKI_USE_HARDWARE_MOCKS})
  message(INFO " Using hardware mocks")
  set(USE_SCRIPT_MOCKS ON)
  set(USE_PIGPIO_MOCK ON)
else()
  message(INFO " Using real hardware")
  set(USE_SCRIPT_MOCKS OFF)
  set(USE_PIGPIO_MOCK OFF)
endif()

# Use dummy RPC handler 
set(USE_HANDLER_MOCK OFF)

set(DAEMON_SCRIPTS_PREFIX /usr/lib/${MODULE_NAME}/daemon/Scripts)

if(USE_SCRIPT_MOCKS)
  set(DAEMON_SCRIPTS_DIR ${DAEMON_SCRIPTS_PREFIX}/Mocks/)
else()
  set(DAEMON_SCRIPTS_DIR ${DAEMON_SCRIPTS_PREFIX}/Impls/)
endif()

add_compile_definitions(DAEMON_SCRIPTS_DIR=${DAEMON_SCRIPTS_DIR})
add_compile_definitions(DAEMON_CLI_EXECUTABLE=${DAEMON_CLI_EXECUTABLE})
add_compile_definitions(DAEMON_PORT_ENV=${DAEMON_PORT_ENV})
add_compile_definitions(DAEMON_LOGFILE_ENV=${DAEMON_LOGFILE_ENV})
add_compile_definitions(DAEMON_BACKLOG_ENV=${DAEMON_BACKLOG_ENV})
add_compile_definitions(DAEMON_CONN_ATTEMPT_COUNT_ENV=${DAEMON_CONN_ATTEMPT_COUNT_ENV})
add_compile_definitions(DAEMON_CONN_ATTEMPT_PERIOD_ENV=${DAEMON_CONN_ATTEMPT_PERIOD_ENV})

if(USE_HANDLER_MOCK)
  add_compile_definitions(USE_HANDLER_MOCK)
endif()

project(roki-mb-daemon)
include_directories(Inc/)

# Provide pigpio
if(USE_PIGPIO_MOCK)
  add_library(pigpio Src/PiGpioStub.cpp)
else()
  find_library(PIGPIO_FOUND pigpio)
  if (NOT PIGPIO_FOUND)
    message(FATAL_ERROR " pigpio library was not found")
  endif()
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
target_link_libraries(DaemonTools PUBLIC Server HandlerMock HandlerImpl CheckDefs Client)
target_include_directories(DaemonTools PUBLIC Inc/)

add_library(CLI Src/CLI.cpp)
target_link_libraries(CLI PUBLIC DaemonTools)

# CLI Executable
add_executable(${DAEMON_CLI_EXECUTABLE} Src/CLIMain.cpp)
target_link_libraries(${DAEMON_CLI_EXECUTABLE} PUBLIC CLI)
install(TARGETS ${DAEMON_CLI_EXECUTABLE} RUNTIME DESTINATION /usr/bin/)

# Script installation
install(DIRECTORY Scripts/ DESTINATION ${DAEMON_SCRIPTS_PREFIX})
