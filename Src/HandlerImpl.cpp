#include "roki-mb-daemon/HandlerImpl.hpp"

using namespace Roki;

using Msgs = RPCDefs::Messages;
using Proc = RPCDefs::Procedures;
using LockGuard = std::lock_guard<std::mutex>;
using State = FirmwareFSM::FWState;

template <typename Msg>
using Responce = IHandler::IResponce<Msg>;

template <typename T>
using ResponcePtr = std::unique_ptr<Responce<T>>;

struct ChipStatusResponce final : public Responce<Msgs::ChipStatus> {
  std::string DescriptionBuf;
  std::string UserNameBuf;
};

struct PortPathResponce final : public Responce<Msgs::String> {
  std::string PathBuf;
};

ResponcePtr<Msgs::ChipStatus> HandlerImpl::GetStatus(const Msgs::Empty&) {
  LockGuard _{*Mutex};

  auto msg = std::make_unique<ChipStatusResponce>();

  msg->DescriptionBuf = Firmware.GetStateDescription();
  msg->Description.Data = msg->DescriptionBuf.c_str();
  msg->Description.Size = msg->DescriptionBuf.size();

  msg->Acquired = Acquired;

  msg->UserNameBuf = UserName;
  msg->User.Name.Data = msg->UserNameBuf.c_str();
  msg->User.Name.Size = msg->UserNameBuf.size();

  msg->User.PID = UserPid;

  return msg;
}

static ResponcePtr<Msgs::Empty> MakeEmptyResponce() {
  return std::make_unique<Responce<Msgs::Empty>>();
}

void HandlerImpl::EnsureNotAcquired(const std::string& procName) {
  LockGuard _{*Mutex};
  if (Acquired)
    throw std::runtime_error("Can't do " + procName + ": Port is acquired by " +
                             UserName + " at PID " + std::to_string(UserPid));
}

ResponcePtr<Msgs::Empty> HandlerImpl::Flash(const Msgs::String& path) {
  EnsureNotAcquired("Flash");
  Firmware.FlashBootlader(path.ToCxxStr());
  return MakeEmptyResponce();
}

ResponcePtr<Msgs::Empty> HandlerImpl::Start(const Msgs::Empty&) {
  Firmware.StartFirmware();
  return MakeEmptyResponce();
}

ResponcePtr<Msgs::Empty> HandlerImpl::Reset(const Msgs::Empty&) {
  EnsureNotAcquired("Reset");
  Firmware.ResetChip();
  return MakeEmptyResponce();
}

ResponcePtr<Msgs::String> HandlerImpl::Connect(const Msgs::UserData& userData) {
  EnsureNotAcquired("Connect");

  LockGuard _{*Mutex};
  auto msg = std::make_unique<PortPathResponce>();

  msg->PathBuf = Firmware.GetPort();

  msg->Data = msg->PathBuf.c_str();
  msg->Size = msg->PathBuf.size();

  UserName = userData.Name.ToCxxStr();
  UserPid = userData.PID;
  Acquired = true;

  return msg;
}

ResponcePtr<Msgs::Empty> HandlerImpl::Disconnect(const Msgs::Empty&) {
  LockGuard _{*Mutex};

  if (!Acquired) throw std::runtime_error("Port is not acquired");
  Acquired = false;

  return MakeEmptyResponce();
}