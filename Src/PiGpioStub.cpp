// This file is meant to provide dummy RPI GPIO
// functions needed to build HwUtils.cpp

#include "PiGpioStub.hpp"

#include <cassert>
#include <stdexcept>

#include "Helpers.hpp"

using namespace Roki::Helpers;

#ifndef PIGPIO_IGNORE
// Make functions throw on calls
#define STUB_IMPL \
  { throw FEXCEPT(std::runtime_error, "Not implemented"); }
#else
#define STUB_IMPL \
  { return {}; }
#endif

int gpioInitialise() STUB_IMPL;
int gpioTerminate() STUB_IMPL;
int gpioSetMode(int, int) STUB_IMPL;
int gpioWrite(int, int) STUB_IMPL;
