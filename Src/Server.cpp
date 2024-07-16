#include "Server.hpp"

using namespace Roki;
using namespace Helpers;

using ForcedUnwind = abi::__forced_unwind;

HelloMessage::HelloMessage(size_t port) : Port{port} {}

void HelloMessage::Send(int fd) {
  char* buf = reinterpret_cast<char*>(&Port);

  size_t size = sizeof(Port);
  size_t total = 0;

  while (total < size) {
    int ret = send(fd, &buf[total], total - size, 0);

    if (ret < 0) throw FEXCEPT(ErrnoException, "Failed to send()", errno);

    assert(ret != 0);

    total += ret;
  }
}

size_t HelloMessage::GetPort() const { return Port; }

auto HelloMessage::Recv(int fd) -> HelloMessage {
  HelloMessage msg;
  char* buf = reinterpret_cast<char*>(&msg.Port);
  size_t size = sizeof(Port);

  size_t total = 0;

  while (total < size) {
    int ret = recv(fd, &buf[total], total - size, 0);

    if (ret < 0) throw FEXCEPT(ErrnoException, "Failed to send()", errno);

    assert(ret != 0);

    total += ret;
  }

  return msg;
}

void Server::CleanupRoutine() {
  static std::string prefix = "Collector: ";

  std::unique_lock lk(JoinablesMutex);

  while (1) {
    ReadyToJoin.wait(lk, [this] { return !Joinables.empty() || DoShutdown; });

    if (DoShutdown && Handlers.empty()) break;

    while (!Joinables.empty()) {
      size_t id = Joinables.front();

      auto& thread = Handlers.at(id);
      assert(thread.joinable());
      thread.join();
      Handlers.erase(id);
      Joinables.pop();

      Logger << prefix << "Closed thread #" << id << std::endl;
    }
  }
}

template <typename Proc>
void Server::GenericHandler(const RPCProvider::MsgHeader& header) {
  auto request = Proc::Request::Deserialize(header.Data);
  auto responce = HandlerImpl<Proc>(request);
  rpc.PackMsg(Proc::ID, responce);
}

void Server::DispatchPackage(const RPCProvider::MsgHeader& header) {
  switch (header.ProcId) {
#define CASEGEN(Proc)                                           \
  case RPCDefs::Procedures::Proc::ID:                           \
    GenericHandler<typename RPCDefs::Procedures::Proc>(header); \
    break;

    CASEGEN(GetStatus);
    CASEGEN(Flash);
    CASEGEN(Start);
    CASEGEN(Connect);
    CASEGEN(Disconnect);
    CASEGEN(Reset);
    CASEGEN(Shutdown);

#undef CASEGEN
  }
}

void Server::BlockAllSignals() {
  sigset_t set;
  sigfillset(&set);
  pthread_sigmask(SIG_BLOCK, &set, NULL);
}

void Server::HandlerRoutine(ServerSocket&& newSocket, HandlerId id) {
  BlockAllSignals();
  Helpers::Defer _{[this, id]() {
    JoinablesMutex.lock();
    Joinables.push(id);
    ReadyToJoin.notify_all();
    JoinablesMutex.unlock();
  }};

  // Disable cancellation for a while
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

  std::string prefix = "Handler #" + std::to_string(id) + ": ";
  ServerSocket socket = std::move(newSocket);

  Logger << prefix << "Waiting for connection on port " << socket.GetPort()
         << "..." << std::endl;

  // TODO implement timeout
  Connection conn = socket.Accept();

  Logger << prefix << "Accepted connection from " << conn.GetAddr()
         << std::endl;

  RPCProvider rpc{std::move(conn), ErrCode};
  bool connOk = true;
  auto flagSetter = [&connOk]() { connOk = false; };

  while (!DoShutdown) try {
      // enable cancellation on first iteration
      static auto _ = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

      Defer recvGuard{flagSetter};
      auto header = rpc.RecvPackage();
      recvGuard.Cancel();

      DispatchPackage(header);

      Defer sendGuard{flagSetter};
      rpc.SendPackage();
      sendGuard.Cancel();
    } catch (std::exception& e) {
      Logger << prefix << "Exception occured: " << e.what() << std::endl;

      if (connOk) {
        rpc.PackError(e.what());
        rpc.SendPackage();
      } else
        break;
    } catch (ForcedUnwind&) {
      Logger << prefix << "Shutdown requested" << std::endl;

      if (connOk) {
        rpc.PackError("Server closed");
        rpc.SendPackage();
      }

      throw;  // It is mandatory for pthread_cancel unwinding
    } catch (...) {
      Logger << prefix << "Unknown exception" << std::endl;

      if (connOk) {
        rpc.PackError("Unknown exception");
        rpc.SendPackage();
      }

      throw;
    }
}

void Server::ShutdownRoutine() {
  DoShutdown = true;
  for (auto& pair : Handlers) {
    pthread_cancel(pair.second.native_handle());
  }
}

ServerSocket Server::CreateHandlerSocket() {
  for (;; CurrentPort++) {
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
  sigset_t set;
  sigfillset(&set);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  static volatile sig_atomic_t shutdownReceived = false;
  auto handler = [](int) { shutdownReceived = true; };

  std::signal(SIGTERM, handler);
  std::signal(SIGINT, handler);

  sigemptyset(&set);
  sigaddset(&set, SIGTERM);
  sigaddset(&set, SIGINT);
  pthread_sigmask(SIG_UNBLOCK, &set, NULL);

  std::thread cleanup{CleanupRoutine, this};

  while (1) {
    try {
      Connection newConnection = Socket.Accept();

      Logger << MainThreadPrefix << "Accepted connection from "
             << newConnection.GetAddr() << std::endl;

      auto newSocket = CreateHandlerSocket();

      Logger << MainThreadPrefix << "Creating handler on port "
             << newSocket.GetPort() << "...";

      size_t newId = CurrentId++;
      std::thread newThread{HandlerRoutine, this, std::move(newSocket), newId};
      Handlers.insert({newId, std::move(newThread)});

      HelloMessage hello{newSocket.GetPort()};
      hello.Send(newConnection.GetFd());

      Logger << MainThreadPrefix << "Handler #" << newId << " created"
             << std::endl;
    } catch (std::exception& e) {
      if (shutdownReceived)
        Logger << MainThreadPrefix << "Shutting down..." << std::endl;
      else
        Logger << MainThreadPrefix << "Exception occured: " << e.what()
               << std::endl;

      break;
    }
  }

  ShutdownRoutine();

  cleanup.join();
}

Server::Server(AddrType addr, size_t port, size_t backlog, std::ostream& logger)
    : Socket{addr, port, backlog}, Logger{logger} {}
