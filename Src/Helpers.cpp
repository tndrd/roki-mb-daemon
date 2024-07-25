#include "roki-mb-daemon/Helpers.hpp"

using namespace MbDaemon::Helpers;

DescriptorWrapper::DescriptorWrapper(int newFd) : Fd{newFd} {}
DescriptorWrapper::~DescriptorWrapper() { close(Fd.Get()); }
int DescriptorWrapper::Get() const { return Fd.Get(); }

ErrnoException::ErrnoException(std::string msg, int error)
    : std::runtime_error{msg + ": " + "errno #" + std::to_string(error) + ": " +
                         strerror(error)},
      Error{error} {}

int ErrnoException::GetErrno() const noexcept { return Error; }

Defer::Defer(std::function<void()> func) : Deferred{func} {}
Defer::~Defer() { Deferred(); }

void Defer::Cancel() {
  Deferred = [] {};
}

std::string MbDaemon::Helpers::IpToString(AddrType addr) {
  sockaddr_in sa;
  sa.sin_addr.s_addr = addr;

  std::string str(INET_ADDRSTRLEN, 0);

  if (inet_ntop(AF_INET, &sa.sin_addr, &str[0], INET_ADDRSTRLEN) == NULL)
    throw FEXCEPT(ErrnoException,
                  "Failed to convert address to string via inet_ntop()", errno);

  return str;
}

AddrType MbDaemon::Helpers::IpFromString(const std::string& str) {
#define MSG "Failed to parse addr from string via inet_pton()"
  sockaddr_in sa;

  int ret = inet_pton(AF_INET, str.c_str(), &sa.sin_addr);

  if (ret == 0)
    throw FEXCEPT(std::runtime_error,
                  MSG ": Invalid address, inet_pton returned 0");

  if (ret < 0) throw FEXCEPT(ErrnoException, MSG, errno);

  return sa.sin_addr.s_addr;
#undef MSG
}

static void MbDaemon::Helpers::SetSigMask(int how) {
  sigset_t newSet;

  sigfillset(&newSet);
  pthread_sigmask(how, &newSet, NULL);
}

void TermTools::MakeRed() { std::cout << "\033[91m"; }
void TermTools::MakeDefault() { std::cout << "\033[39m"; }