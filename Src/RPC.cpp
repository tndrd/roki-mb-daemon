#include "roki-mb-daemon/RPC.hpp"

using namespace Roki;
using namespace Helpers;

void RPCProvider::RecvToBuffer(size_t size) {
  size_t total = 0;
  char* ptr = Buffer.data();

  while (total < size) {
    int ret = recv(Conn.GetFd(), ptr + total, size - total, 0);

    if (ret < 0) throw FEXCEPT(ErrnoException, "Failed to recv()", errno);
    if (ret == 0) throw FEXCEPT(std::runtime_error, "Connection closed");
    total += ret;
  }
}

void RPCProvider::SendFromBuffer(size_t size) {
  size_t total = 0;

  while (total < size) {
    int ret = send(Conn.GetFd(), Buffer.data() + total, size - total, 0);
    if (ret < 0) throw FEXCEPT(ErrnoException, "Failed to send()", errno);
    assert(ret != 0);

    total += ret;
  }
}

void RPCProvider::PackError(const std::string& msg) {
  if (msg.size() > 255)
    throw FEXCEPT(std::runtime_error, "Message to big to fit inside package");

  Buffer[0] = ErrCode;
  Buffer[1] = msg.size();

  memcpy(Buffer.data() + 2, msg.c_str(), msg.size());

  SendFromBuffer(msg.size() + 2);
}

auto RPCProvider::RecvPackage() -> MsgHeader {
  MsgHeader header;

  RecvToBuffer(2);

  header.ProcId = Buffer[0];
  header.Ok = (Buffer[0] != ErrCode);
  header.Size = Buffer[1];

  RecvToBuffer(header.Size);

  if (!header.Ok) {
    header.Msg = std::string(header.Size, 0);
    memcpy(&header.Msg[0], Buffer.data(), header.Size);
  }

  header.Data = Buffer.data();

  return header;
}

void RPCProvider::SendPackage() { SendFromBuffer(Buffer[1] + 2); }

RPCProvider::RPCProvider(Connection&& connection, uint8_t errCode)
    : Conn{std::move(connection)}, ErrCode{errCode} {}