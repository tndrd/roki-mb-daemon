#include "Socket.hpp"

using namespace Roki;
using namespace Helpers;

Connection::Connection(DescriptorWrapper&& fd, const std::string& addr,
                       size_t port, bool nonBlock)
    : Fd{std::move(fd)}, Addr{addr}, Port{port}, NonBlock{nonBlock} {}

std::string Connection::GetAddr() const { return Addr; }
size_t Connection::GetPort() const { return Port.Get(); }
bool Connection::IsNonBlock() const { return NonBlock.Get(); }

int Connection::GetFd() const { return Fd.Get(); }

ServerSocket::ServerSocket(AddrType addr, size_t port, size_t backlog) {
  int newFd = 0;
  sockaddr_in sa;

  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = addr;
  sa.sin_port = htons(port);

  if ((newFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    throw FEXCEPT(ErrnoException, "Failed to open socket", errno);

  ServerAddr = IpToString(addr);
  ServerPort = port;
  Backlog = backlog;
  SockFd = DescriptorWrapper{newFd};

  if (bind(newFd, reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) < 0)
    throw FEXCEPT(ErrnoException, "Failed to bind socket", errno);

  if (listen(newFd, backlog) < 0)
    throw FEXCEPT(ErrnoException, "listen() failed", errno);
}

Connection ServerSocket::Accept(bool nonBlock = false) {
  sockaddr_in sa;
  socklen_t addrlen = sizeof(sa);
  int peerFd = accept4(SockFd.Get(), reinterpret_cast<sockaddr*>(&sa), &addrlen,
                       nonBlock ? SOCK_NONBLOCK : 0);

  if (peerFd < 0) throw FEXCEPT(ErrnoException, "Failed to accept connection");

  DescriptorWrapper retFd{peerFd};
  std::string addrStr = IpToString(sa.sin_addr.s_addr);

  return {std::move(retFd), addrStr, ServerPort, nonBlock};
}

size_t ServerSocket::GetPort() const { return ServerPort; }
std::string ServerSocket::GetAddr() const { return ServerAddr; }

ClientSocket::ClientSocket(AddrType addr, PortType port)
    : AddrIn{addr},
      PortIn{htons(port)},
      ServerAddr{IpToString(addr)},
      ServerPort{port} {}

Connection ClientSocket::Connect(bool nonBlock = false) {
  int newFd = socket(AF_INET, SOCK_STREAM | (nonBlock ? SOCK_NONBLOCK : 0), 0);

  if (newFd < 0)
    throw FEXCEPT(ErrnoException, "Failed to create socket", errno);

  DescriptorWrapper retFd{newFd};

  sockaddr_in sa;
  sa.sin_addr.s_addr = AddrIn;
  sa.sin_family = AF_INET;
  sa.sin_port = PortIn;

  if (connect(newFd, reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) < 0)
    throw FEXCEPT(ErrnoException, "Failed to connect", errno);

  return {std::move(retFd), ServerAddr, ServerPort, nonBlock};
}

size_t ClientSocket::GetPort() const { return ServerPort; }
std::string ClientSocket::GetAddr() const { return ServerAddr; }
