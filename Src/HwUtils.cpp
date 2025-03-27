#include "roki-mb-daemon/HwUtils.hpp"

#define RESET_PIN 6
#define SIGNAL_WIDTH_US 100 * 1000  // 100ms

#define str(a) #a
#define xstr(a) str(a)

#define SCRIPTS_PATH xstr(DAEMON_SCRIPTS_DIR)

#define BOOTLOADER_FLASH SCRIPTS_PATH "BootloaderFlash.py"
#define BOOTLOADER_START SCRIPTS_PATH "BootloaderStart.py"
#define BOOTLOADER_FIND SCRIPTS_PATH "BootloaderFind.py"
#define BOOTLOADER_RESET SCRIPTS_PATH "BootloaderReset.py"

#define PYTHON_EXECUTABLE "python3"

#define FILLSTRINGFROMFD_BUFSIZE 64
#define PYTHON_RUN(script) PYTHON_EXECUTABLE " " script " "

using namespace MbDaemon;
using namespace Helpers;

void HwUtils::ExecuteChildRoutine(const std::string& cmd, int outFd) {
  if (outFd != STDOUT_FILENO) {
    if (dup2(outFd, STDOUT_FILENO) < 0) {
      std::cout << "Failed to redirect output: " << strerror(errno)
                << std::endl;
      exit(1);
    }
  }

  if (execl("/bin/sh", "sh", "-c", cmd.c_str(), NULL) < 0) {
    std::cout << "Failed to run script \"" + cmd + "\": " << strerror(errno)
              << std::endl;
    exit(1);
  }
}

void HwUtils::Execute(const std::string& cmd, int outFd, bool closeFd) {
  int ret = fork();

  if (ret < 0)
    throw FEXCEPT(ErrnoException, "Failed to create child process", errno);

  if (ret == 0) ExecuteChildRoutine(cmd, outFd);

  int status = 0;
  ret = wait(&status);

  if (closeFd) close(outFd);

  if (ret < 0) throw FEXCEPT(ErrnoException, "Failed to wait for child", errno);

  if (!WIFEXITED(status))
    throw FEXCEPT(std::runtime_error, "Flashing script terminated abnormally");

  int exitCode = WEXITSTATUS(status);

  if (exitCode != 0)
    throw FEXCEPT(std::runtime_error,
                  "Flashing script exited with non-zero code " +
                      std::to_string(exitCode));
}

// In previous version this function
// was implemented using libpigpio
// However, it did mess up audio amplifier's I2S,
// resulting in speakers being burnt down.
void HwUtils::ChipReset() {
  Execute(PYTHON_RUN(BOOTLOADER_RESET));
}

void HwUtils::BootloaderFlash(const std::string& firmware) {
  Execute(PYTHON_RUN(BOOTLOADER_FLASH) + firmware);
}

void HwUtils::BootloaderStart() { Execute(PYTHON_RUN(BOOTLOADER_START)); }

std::string HwUtils::BootloaderFind() {
  int pipeFd[2] = {};

  if (pipe(pipeFd) < 0)
    throw FEXCEPT(ErrnoException, "Failed to create pipe", errno);

  int readFd = pipeFd[0];
  int writeFd = pipeFd[1];

  std::string dst;

  Execute(PYTHON_RUN(BOOTLOADER_FIND), writeFd, true);
  FillStringFromFd(dst, readFd);

  close(readFd);

  return dst;
}

void HwUtils::FillStringFromFd(std::string& dst, int srcFd) {
  char buf[FILLSTRINGFROMFD_BUFSIZE + 1] = {};

  size_t nRead = 0;

  while (1) {
    int ret = read(srcFd, buf, sizeof(buf));
    if (ret < 0) throw FEXCEPT(ErrnoException, "read() failed", errno);
    if (ret == 0) break;

    nRead += ret;
    buf[nRead] = 0;

    dst.insert(dst.back(), buf);
  }
}
