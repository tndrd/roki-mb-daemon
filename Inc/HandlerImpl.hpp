#pragma once

#include "Server.hpp"

namespace Roki {
class HandlerImpl final: public IHandler {
  #define RPC RPCDefs::Procedures

  #define PROCEDURE(Proc) RPC::Proc::Responce Proc(const RPC::Proc::Request&) override;
  #include "Procedures.list"
  #undef PROCEDURE
};
}