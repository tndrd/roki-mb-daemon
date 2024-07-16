#pragma once

#include "Client.hpp"
#include "DaemonTools.hpp"
#include "RPCDefs.hpp"

namespace Roki {

class DaemonCLI {
 private:
  using TokenBuf = std::vector<std::string>;
  using TokenInd = size_t;

 public:
  class SyntaxException : public std::runtime_error {
   private:
    size_t TokenIndex;

   public:
    SyntaxException(size_t tokenIndex, const std::string& msg);
    size_t GetTokenIndex() const noexcept;
    virtual ~SyntaxException() = default;
  };

 private:
  struct KeyWords {
#define DECLARE(name, str) static constexpr auto name = str;

    DECLARE(Daemon, "daemon");
    DECLARE(Chip, "chip");
    DECLARE(Flash, "flash");
    DECLARE(Start, "start");
    DECLARE(Stop, "stop");
    DECLARE(Status, "status");
    DECLARE(Help, "help");

#undef DECLARE
  };

 private:
  TokenBuf Tokens;
  TokenInd CurrentToken;
  TokenInd PrevToken;

 private:
  const std::string& GetNextToken();
  void MakeErrorMessage(const std::string& msg) const noexcept;
  void MakeSyntaxErrorMessage(size_t tokenInd,
                              const std::string& msg) const noexcept;
  void UnknownToken();
  void PutDescription(const TokenBuf& tokens,
                               const std::string& description);
  void PrintUsage() const;
  void Run();
  void CheckFile(const std::string& path);
  Client MakeClient();
  void PutDaemonParams(const DaemonTools::Params& params);

  void DoChipStart();
  void DoChipStop();
  void DoChipFlash(const std::string& path);
  void DoDaemonStart();
  void DoDaemonStop();
  void DoHelp();
  void DoStatus();

 public:
  static void Execute(const TokenBuf& tokens);
  static void Execute(int argc, char* argv[]);

  DaemonCLI(const TokenBuf& tokens);
};
}  // namespace Roki