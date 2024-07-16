#include "RPCDefs.hpp"

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
  }
}

auto RPCDefs::Messages::Empty::GetPackedSize() const -> Byte { return 0; }

void RPCDefs::Messages::Empty::Serialize(Byte* ptr) const { assert(ptr); };
auto RPCDefs::Messages::Empty::Deserialize(const Byte* ptr) -> Empty {
  assert(ptr);
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

auto RPCDefs::Messages::String::Deserialize(Byte* ptr) -> String {
  assert(ptr);
  String newString;
  newString.Size = ptr[0];
  newString.Data = ptr + 1;
}

std::string RPCDefs::Messages::String::ToCxxStr() {
  if (!Data) throw FEXCEPT(std::runtime_error, "String data is not defined");

  std::string str(Size, 0);

  memcpy(&str[0], Data, Size);

  return str;
}
