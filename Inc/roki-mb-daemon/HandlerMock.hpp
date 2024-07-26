#pragma once

#include "roki-mb-daemon/Server.hpp"

namespace MbDaemon {
struct HandlerMock final : public IHandler {
#define RPC RPCDefs::Procedures

#define PROCEDURE(Proc)                                 \
  std::unique_ptr<IResponce<RPC::Proc::Responce>> Proc( \
      const RPC::Proc::Request&) override;
#include "roki-mb-daemon/Procedures.list"
#undef PROCEDURE
#undef RPC
};
}  // namespace MbDaemon