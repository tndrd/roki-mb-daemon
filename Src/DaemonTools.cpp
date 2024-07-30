#include <sys/stat.h>
#include <sys/types.h>

#include <roki-mb-daemon/DaemonTools.hpp>

using namespace std::string_literals;
using namespace MbDaemon;
using namespace Helpers;

#define str(a) #a
#define xstr(a) str(a)

void DaemonTools::ReadToBuf(int fd, size_t size) {
  char* buf = Buffer.data();
  size_t total = 0;

  while (total < size) {
    int ret = read(fd, buf + total, size - total);
    if (ret < 0) throw FEXCEPT(ErrnoException, "read()", errno);

    total += ret;
  }
}

void DaemonTools::WriteFromBuf(int fd, size_t size) const {
  const char* buf = Buffer.data();
  size_t total = 0;

  while (total < size) {
    int ret = write(fd, buf + total, size - total);
    if (ret < 0) throw FEXCEPT(ErrnoException, "write()", errno);

    total += ret;
  }
}

void DaemonTools::PutError(int fd, const std::string& msg) {
  size_t msgSz = msg.size();
  Buffer[0] = ~ACK;
  Buffer[1] = (msgSz < BufferSize) ? msgSz : BufferSize;

  memcpy(&Buffer[2], msg.c_str(), msgSz);

  WriteFromBuf(fd, msgSz + 2);
}

void DaemonTools::PutAck(int fd) {
  Buffer[0] = ACK;
  WriteFromBuf(fd, 1);
}

auto DaemonTools::ReadResult(int fd) -> LaunchResult {
  LaunchResult result;

  ReadToBuf(fd, 1);
  if (Buffer[0] != ACK) {
    result.Ok = false;

    ReadToBuf(fd, 1);
    uint8_t msgSz = Buffer[0];

    ReadToBuf(fd, msgSz);
    Buffer[msgSz] = 0;

    result.Msg = Buffer.data();
    result.Pid = -1;
  } else {
    result.Ok = true;
  }

  return result;
}

auto DaemonTools::LaunchAt(const Params& params) -> LaunchResult {
  int pipeFd[2] = {};

  if (pipe(pipeFd) < 0) throw FEXCEPT(ErrnoException, "pipe()", errno);

  int readFd = pipeFd[0];
  int writeFd = pipeFd[1];

  int ret = fork();
  if (ret < 0) throw FEXCEPT(ErrnoException, "fork()", errno);

  // Parent
  if (ret != 0) {
    auto result = ReadResult(readFd);
    result.Pid = ret;
    close(readFd);
    return result;
  }

  // Child
  Defer fdGuard{[writeFd]() { close(writeFd); }};

  umask(0);

  std::ofstream logfile{params.LogPath};

  if (!logfile.good()) {
    PutError(writeFd, "std::ofstream: "s + strerror(errno));
    return {};
  }

  if (daemon(1, 0) < 0) {
    PutError(writeFd, "daemon()"s + strerror(errno));
    return {};
  }

  std::unique_ptr<Server> server = nullptr;

  try {
    server = std::make_unique<Server>(INADDR_ANY, params.Port, params.Backlog,
                                      logfile, MakeHandler());
  } catch (std::exception& e) {
    PutError(writeFd, "Failed to create server: "s + e.what());
    return {};
  }

  PutAck(writeFd);
  fdGuard.Cancel();

  close(writeFd);

  assert(server);

  server->Run();
  exit(0);
}

const char* DaemonTools::GetEnv(const char* env) {
  const char* ptr = std::getenv(env);
  if(!ptr) throw FEXCEPT(std::runtime_error, "Environment variable "s + env + " is not defined");
  return ptr;
}

auto DaemonTools::GetParams() -> Params {
  Params params;

  params.Port = std::stoul(GetEnv(xstr(DAEMON_PORT_ENV)));
  params.Backlog = std::stoul(GetEnv(xstr(DAEMON_BACKLOG_ENV)));
  params.LogPath = GetEnv(xstr(DAEMON_LOGFILE_ENV));

  params.ConnAttemptCount = std::stoul(GetEnv(xstr(DAEMON_CONN_ATTEMPT_COUNT_ENV)));
  params.ConnAttemptPeriod = std::stoul(GetEnv(xstr(DAEMON_CONN_ATTEMPT_PERIOD_ENV)));

  return params;
}

// Maybe there is more clever way
// other than checking if port is busy
bool DaemonTools::IsRunning() {
  auto params = GetParams();

  try {
    ServerSocket dummy{INADDR_ANY, params.Port, params.Backlog};
  } catch (ErrnoException& e) {
    if (e.GetErrno() == EADDRINUSE) return true;
    throw;
  }

  return false;
}

void DaemonTools::Launch() {
  if (IsRunning()) throw FEXCEPT(std::runtime_error, "Daemon already running");

  auto params = GetParams();
  auto result = LaunchAt(params);

  if (!result.Ok)
    throw FEXCEPT(std::runtime_error, "Failed to run daemon: " + result.Msg);
}

void DaemonTools::RunHere() {
  if (IsRunning()) throw FEXCEPT(std::runtime_error, "Daemon already running");

  auto params = GetParams();

  Server server{INADDR_ANY, params.Port, params.Backlog, std::cout,
                MakeHandler()};
  server.Run();
}

std::unique_ptr<IHandler> DaemonTools::MakeHandler() {
  std::unique_ptr<IHandler> handler;

#ifdef USE_HANDLER_MOCK
  handler = std::make_unique<HandlerMock>();
#else
  handler = std::make_unique<HandlerImpl>();
#endif

  return handler;
}

Client DaemonTools::Connect() {
  if (!IsRunning()) throw FEXCEPT(std::runtime_error, "Daemon is not running");

  auto params = GetParams();
  auto factory = Client::Factory{INADDR_ANY, params.Port};

  int i = 0;
  for(;; ++i) try {
    return factory.Connect();
  } catch (std::exception& e) {
    if (i >= params.ConnAttemptCount) throw;
    usleep(params.ConnAttemptPeriod * 1000);
  }
}