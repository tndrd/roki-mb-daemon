#pragma once
#include <netinet/in.h>
#include <sys/socket.h>

#include <Helpers.hpp>
#include <cassert>
#include <cstdlib>

namespace Roki {

struct Connection {
 private:
  Helpers::DescriptorWrapper Fd;
  std::string Addr;
  Helpers::UniqueValue<size_t> Port;
  Helpers::UniqueValue<bool> NonBlock;

 private:
  Connection() = default;

 public:
  Connection(Helpers::DescriptorWrapper&& fd, const std::string& addr,
             size_t port, bool nonBlock);
  std::string GetAddr() const;
  size_t GetPort() const;
  bool IsNonBlock() const;

  int GetFd() const;
};

class ServerSocket {
  Helpers::DescriptorWrapper SockFd;

  std::string ServerAddr;
  size_t ServerPort;
  size_t Backlog;

 public:
  // @addr: address in AF_INET format
  // @port: host-endian port value
  // @backlog: number of pending clients
  ServerSocket(Helpers::AddrType addr, size_t port, size_t backlog);
  Connection Accept(bool nonBlock = false);

  size_t GetPort() const;
  std::string GetAddr() const;
};

class ClientSocket {
 private:
  std::string ServerAddr;
  size_t ServerPort;

  Helpers::AddrType AddrIn;
  Helpers::PortType PortIn;

 public:
  ClientSocket(Helpers::AddrType addr, Helpers::PortType port);

  Connection Connect(bool nonBlock = false);

  size_t GetPort() const;
  std::string GetAddr() const;
};

}  // namespace Roki