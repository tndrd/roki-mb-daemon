#ifndef RPI_FOUND
#pragma message "RPI_FOUND is not set, building with dummy pigpio functions"
#endif

#ifndef SCRIPTS_DIR
#error SCRIPTS_DIR is not set
#endif

#ifndef CLI_EXECUTABLE
#error CLI_EXECUTABLE is not set
#endif

#ifdef USE_SCRIPT_MOCKS
#pragma message "USE_SCRIPT_MOCKS is set, building using script mocks"
#else
#pragma message "USE_SCRIPT_MOCKS is not set, building using script impls"
#endif

#ifdef USE_HANDLER_MOCK
#pragma message "USE_HANDLER_MOCK is set, building using handler mock"
#else
#pragma message "USE_HANDLER_MOCK is not set, building using handler impl"
#endif

#define str(a) #a
#define xstr(a) str(a)

#pragma message "SCRIPTS_DIR = " xstr(SCRIPTS_DIR)
#pragma message "CLI_EXECUTABLE = " xstr(CLI_EXECUTABLE)