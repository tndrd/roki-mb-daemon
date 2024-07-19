#include "HandlerMock.hpp"

using namespace Roki;

#define RPC RPCDefs::Procedures
#define PROCEDURE(Proc)                                   \
  auto HandlerMock::Proc(const RPC::Proc::Request&)       \
      ->std::unique_ptr<IResponce<RPC::Proc::Responce>> { \
    return {};                                            \
  }

#include "Procedures.list"
