#pragma once

#include "roki-mb-daemon/Client.hpp"
#include "roki-mb-daemon/DaemonTools.hpp"
#include "roki-mb-daemon/RPCDefs.hpp"

namespace MbDaemon {

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
    DECLARE(Debug, "debug");
    DECLARE(Block, "block");
    DECLARE(Throw, "throw");
    DECLARE(Connect, "connect");
    DECLARE(Disconnect, "disconnect");
    DECLARE(Kill, "kill");
    DECLARE(Logs, "logs");

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
  void PutDescription(const TokenBuf& tokens, const std::string& description, const std::string& prefix="");
  void PrintUsage() const;
  void Run();
  void CheckFile(const std::string& path);
  Client MakeClient();
  void PutDaemonParams(const DaemonTools::Params& params);

  void DoChipStart();
  void DoChipStop();
  void DoChipFlash(const std::string& path);
  void DoChipStatus();

  void DoDaemonStart();
  void DoDaemonStop();
  void DoDaemonStatus();
  void DoDaemonKill();
  void DoDaemonLogs();

  void DoHelp();

  void DoDaemonDebugStart();
  void DoDaemonDebugBlock();
  void DoDaemonDebugThrow();
  void DoDaemonDebugConnect();
  void DoDaemonDebugDisconnect();

 public:
  static void Execute(const TokenBuf& tokens);
  static void Execute(int argc, char* argv[]);

  DaemonCLI(const TokenBuf& tokens);
};
}  // namespace MbDaemon
