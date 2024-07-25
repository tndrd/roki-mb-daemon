#ifndef PIGPIO_FOUND
#pragma message "PIGPIO_FOUND is not set, building with dummy pigpio functions"
#else
#pragma message "PIGPIO_FOUND is set, building normally"
#endif

#ifndef SCRIPTS_DIR
#error SCRIPTS_DIR is not set
#endif

#ifndef CLI_EXECUTABLE
#error CLI_EXECUTABLE is not set
#endif

#ifndef DAEMON_PORT_ENV
#error DAEMON_PORT_ENV is not defined
#endif

#ifndef DAEMON_LOGFILE_ENV
#error DAEMON_LOGFILE_ENV is not defined
#endif

#ifndef DAEMON_BACKLOG_ENV
#error DAEMON_BACKLOG_ENV is not defined
#endif

#ifdef USE_SCRIPT_MOCKS
#pragma message "USE_SCRIPT_MOCKS is set, building using script mocks"
#else
#pragma message "USE_SCRIPT_MOCKS is not set, building normally"
#endif

#ifdef USE_HANDLER_MOCK
#pragma message "USE_HANDLER_MOCK is set, building using handler mock"
#else
#pragma message "USE_HANDLER_MOCK is not set, building normally"
#endif

#define str(a) #a
#define xstr(a) str(a)

#pragma message "SCRIPTS_DIR = " xstr(SCRIPTS_DIR)
#pragma message "CLI_EXECUTABLE = " xstr(CLI_EXECUTABLE)
#pragma message "DAEMON_PORT_ENV = " xstr(DAEMON_PORT_ENV)
#pragma message "DAEMON_LOGFILE_ENV = " xstr(DAEMON_LOGFILE_ENV)
#pragma message "DAEMON_BACKLOG_ENV = " xstr(DAEMON_BACKLOG_ENV)
