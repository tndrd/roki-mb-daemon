#pragma once

#include <roki-mb-daemon/RPC.hpp>
#include <roki-mb-daemon/RPCDefs.hpp>
#include <roki-mb-daemon/Server.hpp>
#include <roki-mb-daemon/Socket.hpp>

namespace MbDaemon {

class Client {
 private:
  static constexpr uint8_t ErrorCode = RPCDefs::ProcIDs::Error;

 public:
  class Factory {
    ClientSocket Socket;
    Helpers::AddrType INAddr;

   public:
    Factory(Helpers::AddrType addr, Helpers::PortType port);
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
    RPC.PackMsg(Proc::ID, request);
    RPC.SendPackage();

    auto header = RPC.RecvPackage();

    if (!header.Ok) throw ProcException(Proc::ID, header.Msg);

    return Proc::Responce::Deserialize(header.Data);
  }

  void SoftDisconnect();
};

}  // namespace MbDaemon