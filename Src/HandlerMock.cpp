#include "HandlerMock.hpp"

using namespace Roki;

#define RPC RPCDefs::Procedures
#define PROCEDURE(Proc)                                              \
  RPC::Proc::Responce HandlerMock::Proc(const RPC::Proc::Request&) { \
    return {};                                                       \
  }

#include "Procedures.list"
