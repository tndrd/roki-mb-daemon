#pragma once

#include <RPC.hpp>
#include <RPCDefs.hpp>
#include <Server.hpp>
#include <Socket.hpp>

namespace Roki {

using Helpers::Defer;
using Helpers::UniqueValue;

class Client {
 private:
  static constexpr uint8_t ErrorCode = RPCDefs::ProcIDs::Error;

 public:
  class Factory {
    ClientSocket Socket;
    AddrType INAddr;

   public:
    Factory(AddrType addr, size_t port);
    Client Connect();
  };

  class ProcException : public std::runtime_error {
    static std::string Message(RPCDefs::Byte procId, const std::string& error);

   public:
    ProcException(RPCDefs::Byte procId, const std::string& error);
    virtual ~ProcException() = default;
  };

 public:
  using Proc = RPCDefs::Procedures;
  using Msgs = RPCDefs::Messages;

 private:
  RPCProvider RPC;

 private:
  Client(Connection&& conn);

 public:
  template <typename Proc>
  typename Proc::Responce Call(const typename Proc::Request& request) {
    RPC.PackMsg(request);
    RPC.SendPackage();

    auto header = RPC.RecvPackage();

    if (!header.Ok) throw ProcException(header.ProcId, header.Msg);

    return Proc::Responce::Deserialize(header.Data);
  }
};

}  // namespace Roki