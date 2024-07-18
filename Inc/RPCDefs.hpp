#pragma once

#include <netinet/in.h>
#include <sys/socket.h>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>

#include "Helpers.hpp"

namespace Roki {
struct RPCDefs {
  using Byte = char;

  struct ProcIDs {
    static constexpr Byte Error = 0;
    static constexpr Byte Flash = 1;
    static constexpr Byte Start = 2;
    static constexpr Byte Connect = 3;
    static constexpr Byte Disconnect = 4;
    static constexpr Byte Reset = 5;
    static constexpr Byte GetStatus = 8;
    static constexpr Byte Shutdown = 9;
    static constexpr Byte Ping = 10;

    static constexpr Byte DebugThrow = 11;
    static constexpr Byte DebugBlock = 12;

    static const char* ToStr(Byte id);
  };

  struct Messages {
    struct Empty {
      Byte GetPackedSize() const;
      void Serialize(Byte* ptr) const;
      static Empty Deserialize(const Byte* ptr);
    };

    struct String {
      const Byte* Data;
      Byte Size;

      Byte GetPackedSize() const;
      void Serialize(Byte* ptr) const;
      static String Deserialize(const Byte* ptr);

      std::string ToCxxStr();
    };

    struct ChipStatus {
      String Description;
      bool Acquired;
      pid_t UserPID;

      Byte GetPackedSize() const;
      void Serialize(Byte* ptr) const;
      static ChipStatus Deserialize(const Byte* ptr);
    };
  };

  struct Procedures {
    struct GetStatus {
      static constexpr Byte ID = ProcIDs::GetStatus;
      using Request = Messages::Empty;
      using Responce = Messages::ChipStatus;
    };

    struct Flash {
      static constexpr Byte ID = ProcIDs::Flash;
      using Request = Messages::String;
      using Responce = Messages::Empty;
    };

    struct Start {
      static constexpr Byte ID = ProcIDs::Start;
      using Request = Messages::Empty;
      using Responce = Messages::Empty;
    };

    struct Connect {
      static constexpr Byte ID = ProcIDs::Connect;
      using Request = Messages::Empty;
      using Responce = Messages::String;
    };

    struct Disconnect {
      static constexpr Byte ID = ProcIDs::Disconnect;
      using Request = Messages::Empty;
      using Responce = Messages::Empty;
    };

    struct Reset {
      static constexpr Byte ID = ProcIDs::Reset;
      using Request = Messages::Empty;
      using Responce = Messages::Empty;
    };

    struct Shutdown {
      static constexpr Byte ID = ProcIDs::Shutdown;
      using Request = Messages::Empty;
      using Responce = Messages::Empty;
    };

    struct Ping {
      static constexpr Byte ID = ProcIDs::Ping;
      using Request = Messages::Empty;
      using Responce = Messages::Empty;
    };

    struct DebugThrow {
      static constexpr Byte ID = ProcIDs::DebugThrow;
      using Request = Messages::Empty;
      using Responce = Messages::Empty;
    };

    struct DebugBlock {
      static constexpr Byte ID = ProcIDs::DebugBlock;
      using Request = Messages::Empty;
      using Responce = Messages::Empty;
    };
  };
};
}  // namespace Roki