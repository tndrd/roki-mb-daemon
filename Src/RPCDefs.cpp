#include "roki-mb-daemon/RPCDefs.hpp"

using namespace Roki;
using namespace Helpers;

const char* RPCDefs::ProcIDs::ToStr(Byte id) {
  switch (id) {
#define CASEGEN(name) \
  case name:          \
    return #name;
    CASEGEN(Error);
    CASEGEN(Flash);
    CASEGEN(Start);
    CASEGEN(Connect);
    CASEGEN(Disconnect);
    CASEGEN(Reset);
    CASEGEN(GetStatus);
    CASEGEN(Shutdown);
    CASEGEN(Ping);

    CASEGEN(DebugThrow);
    CASEGEN(DebugBlock);

#undef CASEGEN

    default:
      throw std::runtime_error("Unknown procedure id: " + std::to_string(id));
  }
}

auto RPCDefs::Messages::Empty::GetPackedSize() const -> Byte { return 0; }

void RPCDefs::Messages::Empty::Serialize(Byte* ptr) const { assert(ptr); };
auto RPCDefs::Messages::Empty::Deserialize(const Byte* ptr) -> Empty {
  assert(ptr);
  return {};
}

auto RPCDefs::Messages::String::GetPackedSize() const -> Byte {
  return Size + 1;
}

void RPCDefs::Messages::String::Serialize(Byte* ptr) const {
  assert(ptr);
  assert(Size < 255);

  ptr[0] = Size;
  memcpy(ptr + 1, Data, Size);
}

auto RPCDefs::Messages::String::Deserialize(const Byte* ptr) -> String {
  assert(ptr);
  String newString;
  newString.Size = ptr[0];
  newString.Data = ptr + 1;

  return newString;
}

std::string RPCDefs::Messages::String::ToCxxStr() const {
  if (!Data) throw FEXCEPT(std::runtime_error, "String data is not defined");

  std::string str(Size, 0);

  memcpy(&str[0], Data, Size);

  return str;
}

auto RPCDefs::Messages::UserData::GetPackedSize() const -> Byte{
  return Name.GetPackedSize() + sizeof(PID);
}

void RPCDefs::Messages::UserData::Serialize(Byte* ptr) const {
  Name.Serialize(ptr);
  ptr += Name.GetPackedSize();

  *reinterpret_cast<pid_t*>(ptr) = PID;
}

auto RPCDefs::Messages::UserData::Deserialize(const Byte* ptr) -> UserData {
  UserData msg;

  msg.Name = String::Deserialize(ptr);
  ptr += msg.Name.GetPackedSize();

  msg.PID = *reinterpret_cast<const pid_t*>(ptr);

  return msg;
}

auto RPCDefs::Messages::ChipStatus::GetPackedSize() const -> Byte {
  return Description.GetPackedSize() + sizeof(Acquired) + User.GetPackedSize();
}

void RPCDefs::Messages::ChipStatus::Serialize(Byte* ptr) const {
  Description.Serialize(ptr);
  ptr += Description.GetPackedSize();

  *reinterpret_cast<decltype(Acquired)*>(ptr) = Acquired;
  ptr += sizeof(Acquired);

  User.Serialize(ptr);
}

auto RPCDefs::Messages::ChipStatus::Deserialize(const Byte* ptr) -> ChipStatus {
  ChipStatus result;

  result.Description = String::Deserialize(ptr);
  ptr += result.Description.GetPackedSize();

  result.Acquired = *reinterpret_cast<const decltype(result.Acquired)*>(ptr);
  ptr += sizeof(result.Acquired);

  result.User = UserData::Deserialize(ptr);

  return result;
}