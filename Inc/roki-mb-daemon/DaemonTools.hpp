#pragma once

#include <fstream>

#include "roki-mb-daemon/HandlerImpl.hpp"
#include "roki-mb-daemon/HandlerMock.hpp"
#include "roki-mb-daemon/Server.hpp"
#include "roki-mb-daemon/Client.hpp"

namespace MbDaemon {
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
    size_t ConnAttemptCount;
    size_t ConnAttemptPeriod; // ms
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
  void PutAck(int fd);
  LaunchResult ReadResult(int fd);

  LaunchResult LaunchAt(const Params& params);

  static std::unique_ptr<IHandler> MakeHandler();
  static const char* GetEnv(const char* env);

 public:
  DaemonTools() = default;

  static Params GetParams();
  static bool IsRunning();
  void Launch();
  void RunHere();
  Client Connect();
};
}  // namespace MbDaemon