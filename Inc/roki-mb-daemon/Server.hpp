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
  Helpers::PortType Port;

 private:
  HelloMessage() = default;

 public:
  HelloMessage(Helpers::PortType port);

  static HelloMessage Recv(int fd);
  void Send(int fd);
  Helpers::PortType GetPort() const;
};

struct IHandler {
  template <typename T>
  struct IResponce : public T {
    virtual ~IResponce() = default;
  };

#define PROCEDURE(Proc)                                                   \
  virtual std::unique_ptr<IResponce<RPCDefs::Procedures::Proc::Responce>> \
  Proc(const RPCDefs::Procedures::Proc::Request&) = 0;

#include "Procedures.list"

  virtual ~IHandler() = default;

#undef PROCEDURE
};

class Server {
 private:
  using HandlerId = size_t;

 private:
  static constexpr uint8_t ErrCode = RPCDefs::ProcIDs::Error;
  static constexpr size_t StartingPort = 8080;
  static constexpr const char* MainThreadPrefix = "Main thread: ";

 public:
  using PortMsgT = size_t;

 public:
  std::unique_ptr<IHandler> Handler;

  ServerSocket Socket;
  std::ostream& Logger;

  std::unordered_map<HandlerId, std::thread> Handlers;
  std::queue<HandlerId> Joinables;

  std::condition_variable ReadyToJoin;

  std::mutex JoinablesMutex;
  bool DoShutdown = false;

  size_t CurrentPort = StartingPort;
  size_t CurrentId = 0;

  pthread_t TID;

 private:
  template <typename Proc>
  typename std::unique_ptr<IHandler::IResponce<typename Proc::Responce>>
  HandlerImpl(const typename Proc::Request& request);

  template <typename Proc>
  void GenericHandler(RPCProvider& rpc, const RPCProvider::MsgHeader& header,
                      const std::string& prefix);
  void DispatchPackage(RPCProvider& rpc, const RPCProvider::MsgHeader& header,
                       const std::string& prefix);

  static void BlockAllSignals();
  ServerSocket CreateHandlerSocket();

  static void HandlerRoutine(Server* self, ServerSocket&& newSocket,
                             HandlerId id, std::condition_variable&, bool&);
  static void CleanupRoutine(Server* self);

  void RequestShutdown();
  void ShutdownRoutine();

 public:
  void Run();
  Server(Helpers::AddrType addr, Helpers::PortType port, size_t backlog,
         std::ostream& logger, std::unique_ptr<IHandler>&& handler);
};  // namespace Roki

}  // namespace Roki
