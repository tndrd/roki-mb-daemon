// This file is meant to provide dummy RPI GPIO
// functions needed to build HwUtils.cpp

#include "PiGpioStub.hpp"

#define STUB_IMPL \
  { return {}; }

int gpioInitialise() STUB_IMPL;
int gpioTerminate() STUB_IMPL;
int gpioSetMode(int, int) STUB_IMPL;
int gpioWrite(int, int) STUB_IMPL;
