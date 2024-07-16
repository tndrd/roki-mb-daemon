#pragma once
#include <cxxabi.h>

#include <condition_variable>
#include <csignal>
#include <limits>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "FirmwareFSM.hpp"
#include "Helpers.hpp"
#include "RPC.hpp"
#include "RPCDefs.hpp"
#include "Socket.hpp"

namespace Roki {

class HelloMessage {
 private:
  size_t Port;

 private:
  HelloMessage() = default;

 public:
  HelloMessage(size_t port);

  static HelloMessage Recv(int fd);
  void Send(int fd);
  size_t GetPort() const;
};

class Server {
 private:
  using HandlerId = size_t;

 private:
  static constexpr uint8_t ErrCode = RPCDefs::ProcIDs::Error;
  static constexpr size_t StartingPort = 8080;
  static constexpr char* MainThreadPrefix = "Main thread: ";

 public:
  using PortMsgT = size_t;

 public:
  FirmwareFSM Firmware;
  ServerSocket Socket;
  std::ostream& Logger;

  std::unordered_map<HandlerId, std::thread> Handlers;
  std::queue<HandlerId> Joinables;

  std::condition_variable ReadyToJoin;

  std::mutex JoinablesMutex;
  bool DoShutdown = false;

  size_t CurrentPort = StartingPort;
  size_t CurrentId = 0;

 private:
  template <typename Proc>
  typename Proc::Responce HandlerImpl(const typename Proc::Request& request);

  template <typename Proc>
  void GenericHandler(const RPCProvider::MsgHeader& header);
  void DispatchPackage(const RPCProvider::MsgHeader& header);

  static void BlockAllSignals();
  ServerSocket CreateHandlerSocket();

  void HandlerRoutine(ServerSocket&& newSocket, HandlerId id);
  void ShutdownRoutine();
  void CleanupRoutine();

 public:
  void Run();
  Server(AddrType addr, size_t port, size_t backlog, std::ostream& logger);
};  // namespace Roki

}  // namespace Roki