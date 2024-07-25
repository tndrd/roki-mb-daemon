#pragma once

#include <asm/termbits.h>
#include <errno.h>
#include <fcntl.h>

#ifdef PIGPIO_FOUND
#include <pigpio.h>
#else
#include <roki-mb-daemon/PiGpioStub.hpp>
#endif

#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <roki-mb-daemon/Helpers.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

namespace Roki {

class HwUtils {
 private:
  static void ExecuteChildRoutine(const std::string& cmd, int outFd);
  static void Execute(const std::string& cmd, int outFd = STDOUT_FILENO,
                      bool closeFd = false);
  static void FillStringFromFd(std::string& dst, int srcFd);

 public:
  static void ChipReset();
  static void BootloaderFlash(const std::string& firmware);
  static void BootloaderStart();
  static std::string BootloaderFind();
};
}  // namespace Roki
