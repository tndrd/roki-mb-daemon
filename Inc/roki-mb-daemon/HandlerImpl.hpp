#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include "roki-mb-daemon/Server.hpp"

namespace MbDaemon {
class HandlerImpl final : public IHandler {
 private:
  FirmwareFSM Firmware;
  std::unique_ptr<std::mutex> Mutex;

  bool Acquired = false;
  std::string UserName;
  pid_t UserPid = -1;

 private:
  void EnsureNotAcquired(const std::string& prefix);

 public:
  HandlerImpl() : IHandler{}, Mutex{std::make_unique<std::mutex>()} {}

#define RPC RPCDefs::Procedures

#define PROCEDURE(Proc)                                 \
  std::unique_ptr<IResponce<RPC::Proc::Responce>> Proc( \
      const RPC::Proc::Request&) override;
#include "roki-mb-daemon/Procedures.list"
#undef PROCEDURE
#undef RPC
};
}  // namespace MbDaemon