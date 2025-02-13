#pragma once

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include <csignal>
#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <utility>

namespace MbDaemon {

namespace Helpers {

template <typename T, T DefaultValue = T{}>
class UniqueValue {
  T Value;

 public:
  explicit UniqueValue(T newValue) : Value{newValue} {}

  UniqueValue(const UniqueValue&) = delete;
  UniqueValue& operator=(const UniqueValue&) = delete;

  UniqueValue(UniqueValue&& rhs) : Value{std::move(rhs.Value)} {
    rhs.Value = DefaultValue;
  }

  UniqueValue& operator=(UniqueValue&& rhs) {
    std::swap(Value, rhs.Value);
    return *this;
  }

  const T& Get() const { return Value; }

  static UniqueValue Create(T value) { return UniqueValue{value}; }
};

class DescriptorWrapper {
  UniqueValue<int, -1> Fd;

 public:
  explicit DescriptorWrapper(int newFd);
  ~DescriptorWrapper();

  DescriptorWrapper(const DescriptorWrapper&) = delete;
  DescriptorWrapper& operator=(const DescriptorWrapper&) = delete;

  DescriptorWrapper(DescriptorWrapper&& rhs) = default;
  DescriptorWrapper& operator=(DescriptorWrapper&& rhs) = default;

  int Get() const;
};

struct ErrnoException : public std::runtime_error {
 private:
  int Error;

 public:
  ErrnoException(std::string msg, int error);
  virtual ~ErrnoException() = default;

  int GetErrno() const noexcept;
};

template <typename Base>
class PrefixException : public Base {
  std::string Buffer;

 public:
  template <typename... Args>
  PrefixException(const std::string& prefix, Args&&... args)
      : Base{std::forward<Args>(args)...},
        Buffer{prefix + ": " + Base::what()} {}

  const char* what() const noexcept override { return Buffer.c_str(); }

  virtual ~PrefixException() = default;
};

#define FEXCEPT(base, ...) PrefixException<base>(__func__, __VA_ARGS__)

class Defer {
  std::function<void()> Deferred;

 public:
  Defer(std::function<void()> func);
  ~Defer();

  void Cancel();

  Defer(const Defer&) = delete;
  Defer& operator=(const Defer&) = delete;

  Defer(Defer&&) = delete;
  Defer& operator=(Defer&&) = delete;
};

using AddrType = decltype(sockaddr_in::sin_addr.s_addr);
using PortType = decltype(sockaddr_in::sin_port);

std::string IpToString(AddrType addr);
AddrType IpFromString(const std::string& str);
static void SetSigMask(int how);

struct TermTools {
  static void MakeRed();
  static void MakeDefault();
};

using LockGuard = std::lock_guard<std::mutex>;

void ClearPort(const std::string& path);

}  // namespace Helpers
}  // namespace MbDaemon