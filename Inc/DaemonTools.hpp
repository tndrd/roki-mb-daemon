#pragma once

#include <fstream>

#include "Server.hpp"

#define DAEMON_PORT_ENV "MB_DAEMON_PORT"
#define DAEMON_LOGFILE_ENV "MB_DAEMON_LOGFILE"
#define DAEMON_BACKLOG_ENV "MB_DAEMON_BACKLOG"

namespace Roki {
class DaemonTools {
 private:
  static constexpr size_t BufferSize = 256;
  static constexpr size_t BufferExtraSize = 3;
  static constexpr uint8_t ACK = 'y';

 public:
  struct Params {
    Helpers::PortType Port;
    size_t Backlog;
    std::string LogPath;
  };

  struct LaunchResult {
    bool Ok;
    std::string Msg;
    pid_t Pid;
  };

 private:
  // Reserve 3 bytes
  std::array<char, BufferSize + BufferExtraSize> Buffer;

 private:
  void ReadToBuf(int fd, size_t size);
  void WriteFromBuf(int fd, size_t size) const;
  void PutError(int fd, const std::string& msg);
  LaunchResult ReadResult(int fd);

  LaunchResult LaunchAt(const Params& params);

 public:
  DaemonTools() = default;

  static Params GetParams();
  static bool IsRunning();
  void Launch();
  void RunHere();
};
}  // namespace Roki