#include "Client.hpp"

using namespace Roki;

using namespace Helpers;

Client::Factory::Factory(AddrType addr, PortType port)
    : Socket{addr, port}, INAddr{addr} {}

Client Client::Factory::Connect() {
  Connection conn = Socket.Connect();

  auto hello = HelloMessage::Recv(conn.GetFd());
  ClientSocket newSocket(INAddr, hello.GetPort());

  return Client{newSocket.Connect()};
}

std::string Client::ProcException::Message(RPCDefs::Byte procId,
                                           const std::string& error) {
  std::string msg = "Remote procedure \"";
  msg += RPCDefs::ProcIDs::ToStr(procId);
  msg += "\" returned error: " + error;

  return msg;
}

Client::ProcException::ProcException(RPCDefs::Byte procId,
                                     const std::string& error)
    : std::runtime_error{Message(procId, error)} {}

Client::Client(Connection&& conn) : RPC{std::move(conn), ErrorCode} {}

void Client::SoftDisconnect() {
  RPC.PackError("Disconnect");
  RPC.SendPackage();
}