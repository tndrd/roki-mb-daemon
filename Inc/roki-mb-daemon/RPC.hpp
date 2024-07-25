#pragma once

#include "Helpers.hpp"
#include "Socket.hpp"

namespace MbDaemon {

class RPCProvider {
 private:
  static constexpr size_t BufSize = 256;

 public:
  struct MsgHeader {
    bool Ok = true;
    std::string Msg = "OK";
    uint8_t ProcId;

    const char* Data = nullptr;
    uint8_t Size = 0;
  };

 private:
  Connection Conn;
  std::array<char, BufSize> Buffer;
  uint8_t ErrCode;

 private:
  void RecvToBuffer(size_t size);
  void SendFromBuffer(size_t size);

 public:
  template <typename MsgT>
  void PackMsg(uint8_t procId, const MsgT& msg) {
    size_t msgSz = msg.GetPackedSize();

    Buffer[0] = procId;
    Buffer[1] = msgSz;
    msg.Serialize(Buffer.data() + 2);
  }

  void PackError(const std::string& msg);

  MsgHeader RecvPackage();
  void SendPackage();

 public:
  RPCProvider(Connection&& connection, uint8_t errCode);
};
}  // namespace MbDaemon