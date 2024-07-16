// This file is meant to provide dummy RPI GPIO
// functions needed to build HwUtils.cpp

#include "PiGpioStub.hpp"

#include <cassert>
#include <stdexcept>

// Make functions throw on calls
#define NOT_IMPLEMENTED \
  { throw std::runtime_error("Not implemented"); }

int gpioInitialize() NOT_IMPLEMENTED;
int gpioTerminate() NOT_IMPLEMENTED;
int gpioSetMode(int, int) NOT_IMPLEMENTED;
int gpioWrite(int, int) NOT_IMPLEMENTED;