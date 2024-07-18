#pragma once

#include <asm/termbits.h>
#include <errno.h>
#include <fcntl.h>

#ifdef RPI_FOUND
#include <pigpio.h>
#else
#include <PiGpioStub.hpp>
#pragma GCC diagnostics warning \
    "RPI_FOUND is not set, building with dummy pigpio functions"
#endif

#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <Helpers.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

#define RESET_PIN 6
#define SIGNAL_WIDTH_US 100 * 1000  // 100ms

#define SCRIPTS_PATH "Scripts/"

#define BOOTLOADER_FLASH SCRIPTS_PATH "BootloaderFlash.py"
#define BOOTLOADER_START  SCRIPTS_PATH "BootloaderStart.py"
#define BOOTLOADER_FIND SCRIPTS_PATH "BootloaderFind.py"

#define PYTHON_EXECUTABLE "python3"

namespace Roki {

class HwUtils {
 private:
  static void ExecuteChildRoutine(const std::string& cmd, int outFd);
  static void Execute(const std::string& cmd, int outFd = STDOUT_FILENO, bool closeFd = false);
  static void FillStringFromFd(std::string& dst, int srcFd);

 public:
  static void ChipReset();
  static void BootloaderFlash(const std::string& firmware);
  static void BootloaderStart();
  static std::string BootloaderFind();
};
}  // namespace Roki