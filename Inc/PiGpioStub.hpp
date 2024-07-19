// This file is meant to provide dummy RPI GPIO
// functions needed to build HwUtils.cpp

#pragma once

#define PI_OUTPUT 0

int gpioInitialise();
int gpioTerminate();
int gpioSetMode(int, int);
int gpioWrite(int, int);
