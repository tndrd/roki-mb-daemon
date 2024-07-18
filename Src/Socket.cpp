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

void ServerSocket::DisableLingering() {
  linger arg;
  arg.l_onoff = 1;
  arg.l_linger = 0;

  int ret = setsockopt(SockFd.Get(), SOL_SOCKET, SO_LINGER, &arg, sizeof(arg));
  if (ret == 0) return;

  throw FEXCEPT(ErrnoException, "Failed to disable lingering via setsockopt()", errno);
}

ServerSocket::ServerSocket(AddrType addr, size_t port, size_t backlog)
    : SockFd{-1} {
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

Connection ServerSocket::Accept(bool nonBlock) {
  sockaddr_in sa;
  socklen_t addrlen = sizeof(sa);
  int peerFd = accept4(SockFd.Get(), reinterpret_cast<sockaddr*>(&sa), &addrlen,
                       nonBlock ? SOCK_NONBLOCK : 0);

  if (peerFd < 0)
    throw FEXCEPT(ErrnoException, "Failed to accept connection", errno);

  DescriptorWrapper retFd{peerFd};
  std::string addrStr = IpToString(sa.sin_addr.s_addr);

  return {std::move(retFd), addrStr, ServerPort, nonBlock};
}

PortType ServerSocket::GetPort() const { return ServerPort; }
std::string ServerSocket::GetAddr() const { return ServerAddr; }

ClientSocket::ClientSocket(AddrType addr, PortType port)
    : AddrIn{addr}, PortIn{htons(port)}, ServerAddr{IpToString(addr)} {}

Connection ClientSocket::Connect(bool nonBlock) {
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

  return {std::move(retFd), ServerAddr, PortIn, nonBlock};
}

PortType ClientSocket::GetPort() const { return PortIn; }
std::string ClientSocket::GetAddr() const { return ServerAddr; }
