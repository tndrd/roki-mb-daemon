#pragma once

#include "Server.hpp"

namespace Roki {
struct HandlerMock final: public IHandler {
  #define RPC RPCDefs::Procedures

  #define PROCEDURE(Proc) std::unique_ptr<IResponce<RPC::Proc::Responce>> Proc(const RPC::Proc::Request&) override;
  #include "Procedures.list"
  #undef PROCEDURE
};
}