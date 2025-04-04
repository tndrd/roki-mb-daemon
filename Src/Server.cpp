#include "roki-mb-daemon/Server.hpp"

using namespace MbDaemon;
using namespace Helpers;

using ForcedUnwind = abi::__forced_unwind;

HelloMessage::HelloMessage(PortType port) : Port{port} {}

void HelloMessage::Send(int fd) {
  char* buf = reinterpret_cast<char*>(&Port);

  size_t size = sizeof(Port);
  size_t total = 0;

  while (total < size) {
    int ret = send(fd, &buf[total], size - total, 0);

    if (ret < 0) throw FEXCEPT(ErrnoException, "Failed to send()", errno);
    assert(ret != 0);

    total += ret;
  }
}

PortType HelloMessage::GetPort() const { return Port; }

auto HelloMessage::Recv(int fd) -> HelloMessage {
  HelloMessage msg;
  char* buf = reinterpret_cast<char*>(&msg.Port);
  size_t size = sizeof(Port);

  size_t total = 0;

  while (total < size) {
    int ret = recv(fd, &buf[total], size - total, 0);

    if (ret < 0) throw FEXCEPT(ErrnoException, "Failed to recv()", errno);
    if (ret == 0) throw FEXCEPT(std::runtime_error, "Server closed connection");

    total += ret;
  }

  return msg;
}

void Server::CleanupRoutine(Server* self) {
  static std::string prefix = "CleanupRoutine: ";
  assert(self);

  BlockAllSignals();

  Server& server = *self;

  server.Logger << prefix << "Started" << std::endl;

  while (1) {
    std::unique_lock<std::mutex> lk(server.JoinablesMutex);

    server.ReadyToJoin.wait(lk, [&server] {
      return !server.Joinables.empty() || server.DoShutdown;
    });

    if (server.DoShutdown && server.Handlers.empty()) {
      server.Logger << prefix << "Shutting down" << std::endl;
      break;
    }

    while (!server.Joinables.empty()) {
      size_t id = server.Joinables.front();

      auto& thread = server.Handlers.at(id);
      assert(thread.joinable());
      thread.join();
      server.Handlers.erase(id);
      server.Joinables.pop();

      server.Logger << prefix << "Closed thread #" << id << std::endl;
    }
  }
}

#define RPC RPCDefs::Procedures
#define OVERRIDE_HANDLER(Proc)                                                \
  template <>                                                                 \
  typename std::unique_ptr<IHandler::IResponce<typename RPC::Proc::Responce>> \
  Server::HandlerImpl<RPC::Proc>(const typename RPC::Proc::Request& request)

OVERRIDE_HANDLER(Shutdown) {
  pthread_kill(TID, SIGTERM);
  return {};
}

OVERRIDE_HANDLER(Ping) { return {}; }

OVERRIDE_HANDLER(DebugBlock) {
  std::this_thread::sleep_for(std::chrono::seconds(5));
  return {};
}

OVERRIDE_HANDLER(DebugThrow) { throw std::runtime_error("Debug exception"); }

#undef OVERRIDE_HANDLER

#define PROCEDURE(Proc)                                                        \
  template <>                                                                  \
  typename std::unique_ptr<IHandler::IResponce<typename RPC::Proc::Responce>>  \
  Server::HandlerImpl<RPC::Proc>(const typename RPC::Proc::Request& request) { \
    return Handler->Proc(request);                                             \
  }

#include "roki-mb-daemon/Procedures.list"

#undef PROCEDURE
#undef RPC

template <typename Proc>
void Server::GenericHandler(RPCProvider& rpc,
                            const RPCProvider::MsgHeader& header,
                            const std::string& prefix) {
  Logger << prefix << "Performing " << RPCDefs::ProcIDs::ToStr(Proc::ID)
         << std::endl;

  auto request = Proc::Request::Deserialize(header.Data);
  auto responce = HandlerImpl<Proc>(request);
  rpc.PackMsg(Proc::ID, *responce);
}

void Server::DispatchPackage(RPCProvider& rpc,
                             const RPCProvider::MsgHeader& header,
                             const std::string& prefix) {
  switch (header.ProcId) {
#define PROCEDURE(Proc)                                                      \
  case RPCDefs::Procedures::Proc::ID:                                        \
    GenericHandler<typename RPCDefs::Procedures::Proc>(rpc, header, prefix); \
    break;

#include "roki-mb-daemon/Procedures.list"

    PROCEDURE(Shutdown);
    PROCEDURE(Ping);
    PROCEDURE(DebugBlock)
    PROCEDURE(DebugThrow);

#undef CASEGEN
  }
}

void Server::BlockAllSignals() {
  sigset_t set;
  sigfillset(&set);
  pthread_sigmask(SIG_BLOCK, &set, NULL);
}

void Server::HandlerRoutine(Server* self, ServerSocket&& newSocket,
                            HandlerId id, std::condition_variable& readyCond,
                            bool& ready) {
  BlockAllSignals();
  assert(self);

  Server& server = *self;

  Helpers::Defer _{[&server, id]() {
    server.JoinablesMutex.lock();
    server.Joinables.push(id);
    server.ReadyToJoin.notify_all();
    server.JoinablesMutex.unlock();
  }};

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

  std::string prefix = "Handler #" + std::to_string(id) + ": ";
  ServerSocket socket = std::move(newSocket);

  server.Logger << prefix << "Waiting for connection on port "
                << socket.GetPort() << "..." << std::endl;

  std::unique_ptr<Connection> conn;

  ready = true;
  readyCond.notify_all();

  try {
    // TODO implement timeout
    conn = std::make_unique<Connection>(socket.Accept());

  } catch (std::exception& e) {
    server.Logger << prefix << "Failed to accept connection: " << e.what()
                  << std::endl;
    return;
  } catch (ForcedUnwind&) {
    server.Logger << prefix << "Shutdown requested during connection"
                  << std::endl;
    throw;
  } catch (...) {
    server.Logger << prefix << "Unknown exception" << std::endl;
    return;
  }

  assert(conn.get());

  server.Logger << prefix << "Accepted connection from " << conn->GetAddr()
                << std::endl;

  RPCProvider rpc{std::move(*conn), ErrCode};
  bool connOk = true;
  auto flagSetter = [&connOk]() { connOk = false; };

  while (!server.DoShutdown) try {
      Defer recvGuard{flagSetter};
      auto header = rpc.RecvPackage();
      recvGuard.Cancel();

      if (!header.Ok) {
        server.Logger << prefix << "Client disconnected" << std::endl;
        break;
      }

      server.DispatchPackage(rpc, header, prefix);

      Defer sendGuard{flagSetter};
      rpc.SendPackage();
      sendGuard.Cancel();
    } catch (std::exception& e) {
      server.Logger << prefix << "Exception occured: " << e.what() << std::endl;

      if (connOk) {
        rpc.PackError(e.what());
        rpc.SendPackage();
      } else
        break;
    } catch (ForcedUnwind&) {
      server.Logger << prefix << "Shutdown requested" << std::endl;

      if (connOk) {
        rpc.PackError("Server closed");
        rpc.SendPackage();
      }

      throw;  // It is mandatory for pthread_cancel unwinding
    } catch (...) {
      server.Logger << prefix << "Unknown exception" << std::endl;

      if (connOk) {
        rpc.PackError("Unknown exception");
        rpc.SendPackage();
      }

      throw;
    }
}

ServerSocket Server::CreateHandlerSocket() {
  for (;;) {
    CurrentPort++;

    try {
      ServerSocket newSocket{INADDR_ANY, CurrentPort, 1};
      return newSocket;
    } catch (std::exception& e) {
      Logger << MainThreadPrefix << "Failed to create socket on port "
             << CurrentPort << ": " << e.what() << std::endl;
      sleep(1);  // TODO remove after finished
    }
  }
}

void Server::Run() {
  Logger << MainThreadPrefix << "Starting..." << std::endl;
  Logger << MainThreadPrefix << "Main thread PID is " << getpid() << std::endl;

  sigset_t set;
  sigfillset(&set);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  Logger << MainThreadPrefix << "Starting dispatch thread..." << std::endl;
  std::thread dispatch([this]{DispatchRoutine();});

  Logger << MainThreadPrefix << "Starting cleanup thread..." << std::endl;
  std::thread cleanup(Server::CleanupRoutine, this);

  Logger << MainThreadPrefix << "Setting up signal handlers..." << std::endl;

  static Server* self;
  self = this;
  auto handler = [](int) { self->DoShutdown = true; };

  std::signal(SIGTERM, handler);
  std::signal(SIGINT, handler);

  sigemptyset(&set);
  sigaddset(&set, SIGTERM);
  sigaddset(&set, SIGINT);
  pthread_sigmask(SIG_UNBLOCK, &set, NULL);

  while (!DoShutdown) pause();

  // Shutdown dispatch
  Logger << MainThreadPrefix << "Stopping dispatch..." << std::endl;
  pthread_cancel(dispatch.native_handle());
  dispatch.join();

  // Shutdown handlers
  Logger << MainThreadPrefix << "Stopping handlers..." << std::endl;
  for (auto& pair : Handlers) {
    int ret = pthread_cancel(pair.second.native_handle());

    if (ret == 0) continue;

    Logger << MainThreadPrefix << "Warning: Failed to cancel handler #"
           << pair.first << ": " << "errno #" << ret << ": " << strerror(ret)
           << std::endl;
  }
  ReadyToJoin.notify_all();

  // Wait for cleanup to join handlers
  Logger << MainThreadPrefix << "Cleaning up..." << std::endl;
  cleanup.join();

  Logger << MainThreadPrefix << "Shutdown complete" << std::endl;
}

void Server::DispatchRoutine() {
  static const char* prefix = "Dispatch: ";

  Logger << prefix << "Waiting for connections..." << std::endl;
  while (!DoShutdown) {
    try {
      Connection helloConn = Socket.Accept();

      Logger << prefix << "Accepted connection from "
             << helloConn.GetAddr() << std::endl;

      size_t newId = CurrentId++;
      auto handlerSocket = CreateHandlerSocket();

      Logger << prefix << "Creating handler on port "
             << handlerSocket.GetPort() << "..." << std::endl;

      std::mutex readyMutex;
      std::unique_lock<std::mutex> lock{readyMutex};
      std::condition_variable readyCond;
      bool ready = false;

      std::thread handlerThread(Server::HandlerRoutine, this,
                                std::move(handlerSocket), newId,
                                std::ref(readyCond), std::ref(ready));

      JoinablesMutex.lock();
      Handlers.insert({newId, std::move(handlerThread)});
      JoinablesMutex.unlock();

      readyCond.wait(lock, [&ready] { return ready; });

      HelloMessage hello{handlerSocket.GetPort()};
      hello.Send(helloConn.GetFd());

      Logger << prefix << "Handler #" << newId << " created"
             << std::endl;
    } catch (std::exception& e) {
      Logger << prefix << "Exception occured: " << e.what()
             << std::endl;
      break;
    } catch (ForcedUnwind&) {
      Logger << prefix << "Shutdown received" << std::endl;
      throw;
    }
  }
}

Server::Server(AddrType addr, Helpers::PortType port, size_t backlog,
               std::ostream& logger, std::unique_ptr<IHandler>&& handler)
    : Socket{addr, port, backlog},
      Logger{logger},
      TID{pthread_self()},
      Handler{std::move(handler)} {
  Socket.DisableLingering();
}
