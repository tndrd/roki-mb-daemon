#include "CLI.hpp"

#ifndef CLI_EXECUTABLE
#pragma GCC diaginostic error "Executable name is not specified"

// Define symbol to supress IntelliSense error
#define CLI_EXECUTABLE NAME
#endif

#define STRINGIFY(a) #a
#define XSTRINGIFY(a) STRINGIFY(a)

#define EXECUTABLE_NAME XSTRINGIFY(CLI_EXECUTABLE)

#define TAB " - "

using namespace std::string_literals;
using namespace Roki;
using namespace Helpers;

DaemonCLI::SyntaxException::SyntaxException(size_t tokenIndex,
                                            const std::string& msg)
    : std::runtime_error{msg}, TokenIndex{tokenIndex} {}

size_t DaemonCLI::SyntaxException::GetTokenIndex() const noexcept {
  return TokenIndex;
}

const std::string& DaemonCLI::GetNextToken() {
  if (CurrentToken == Tokens.size())
    throw SyntaxException{PrevToken, "Unexpected end of command"};
  PrevToken = CurrentToken;
  return Tokens[CurrentToken++];
}

void DaemonCLI::MakeErrorMessage(const std::string& msg) const noexcept {
  std::cout << "Error: " << msg << std::endl;
}

void DaemonCLI::MakeSyntaxErrorMessage(size_t tokenInd,
                                       const std::string& msg) const noexcept {
  std::cout << "Syntax error: " << msg << std::endl;
  std::cout << "Reason: ";

  for (int i = 0; i < Tokens.size(); ++i) {
    if (i == tokenInd) {
      TermTools::MakeRed();
      std::cout << "=> ";
    }

    std::cout << Tokens[i];

    if (i == tokenInd) {
      std::cout << " <=";
      TermTools::MakeDefault();
    }

    std::cout << " ";
  }

  std::cout << std::endl;

  PrintUsage();

  std::cout << "See \"" << EXECUTABLE_NAME << " " << KeyWords::Help << "\""
            << std::endl;
}

void DaemonCLI::UnknownToken() {
  throw SyntaxException(PrevToken, "Unknown token");
}

void DaemonCLI::Run() try {
  // Skipping first argv
  // (name of executable)
  GetNextToken();

  std::string cmd = GetNextToken();
  if (cmd == KeyWords::Chip) {
    cmd = GetNextToken();

    if (cmd == KeyWords::Start) return DoChipStart();
    if (cmd == KeyWords::Stop) return DoChipStop();
    if (cmd == KeyWords::Flash) return DoChipFlash(GetNextToken());

    UnknownToken();
  }

  if (cmd == KeyWords::Daemon) {
    cmd = GetNextToken();

    if (cmd == KeyWords::Start) return DoDaemonStart();
    if (cmd == KeyWords::Stop) return DoDaemonStop();
    if (cmd == KeyWords::Debug) return DoDaemonDebug();

    UnknownToken();
  }

  if (cmd == KeyWords::Help) return DoHelp();
  if (cmd == KeyWords::Status) return DoStatus();

  UnknownToken();
} catch (SyntaxException& e) {
  MakeSyntaxErrorMessage(e.GetTokenIndex(), e.what());
} catch (std::exception& e) {
  MakeErrorMessage(e.what());
}

DaemonCLI::DaemonCLI(const TokenBuf& tokens)
    : Tokens{tokens}, CurrentToken{0} {}

Client DaemonCLI::MakeClient() {
  DaemonTools daemon;

  if (!daemon.IsRunning()) throw std::runtime_error("Daemon is not running");

  Client::Factory factory{INADDR_ANY, daemon.GetParams().Port};
  return factory.Connect();
}

void DaemonCLI::DoChipStart() {
  Client client = MakeClient();
  client.Call<Client::Proc::Start>({});
  client.SoftDisconnect();
}

void DaemonCLI::DoChipStop() {
  Client client = MakeClient();
  client.Call<Client::Proc::Reset>({});
  client.SoftDisconnect();
}

void DaemonCLI::CheckFile(const std::string& path) {
  int ret = open(path.c_str(), O_RDONLY);
  if (ret < 0)
    throw FEXCEPT(ErrnoException, "Failed to open firmware file", errno);
  close(ret);
}

void DaemonCLI::DoChipFlash(const std::string& path) {
  Client client = MakeClient();

  Client::Msgs::String req;

  req.Data = path.c_str();
  req.Size = path.size();

  client.Call<Client::Proc::Flash>(req);
  client.SoftDisconnect();
}

void DaemonCLI::PutDaemonParams(const DaemonTools::Params& params) {
  std::cout << "Daemon parameters: " << std::endl;
  std::cout << TAB "Port:    " << params.Port << std::endl;
  std::cout << TAB "Backlog: " << params.Backlog << std::endl;
  std::cout << TAB "Logs at: " << params.LogPath << std::endl;
}

void DaemonCLI::DoDaemonStart() {
  DaemonTools daemon;
  daemon.Launch();

  std::cout << "Daemon launched successfully" << std::endl;
  PutDaemonParams(daemon.GetParams());
}

void DaemonCLI::DoDaemonStop() {
  Client client = MakeClient();
  client.Call<Client::Proc::Shutdown>({});
  client.SoftDisconnect();
}

void DaemonCLI::DoDaemonDebug() { DaemonTools{}.RunHere(); }

void DaemonCLI::PutDescription(const TokenBuf& tokens,
                               const std::string& description) {
  assert(tokens.size() > 0);

  std::cout << TAB "\"" EXECUTABLE_NAME " ";

  for (int i = 0; i < tokens.size() - 1; ++i) std::cout << tokens[i] << " ";

  std::cout << tokens.back();

  std::cout << "\": " << description << std::endl;
}

void DaemonCLI::PrintUsage() const {
  std::cout << "Usage: " EXECUTABLE_NAME " <COMMAND> <ARGS>" << std::endl;
}

void DaemonCLI::DoHelp() {
  using KW = KeyWords;

  std::cout << "Roki Motherboard Firmware Manager CLI" << std::endl;
  std::cout << "Author: Lekhterev V.V. @tndrd, Starkit 2024"
               "\n"
            << std::endl;
  PrintUsage();
  std::cout << std::endl << "List of available commands: " << std::endl;

  PutDescription({KW::Chip, KW::Start}, "Starts firmware");
  PutDescription({KW::Chip, KW::Stop}, "Stops firmware");
  PutDescription({KW::Chip, KW::Flash, "<PATH>"}, "Flashes selected firmware");

  PutDescription({KW::Daemon, KW::Start}, "Starts daemon");
  PutDescription({KW::Daemon, KW::Stop}, "Stops daemon");

  PutDescription({KW::Help}, "Prints this text");
  PutDescription({KW::Status}, "Prints status info on daemon and firmware");

  PutDescription({KW::Daemon, KW::Debug}, "Launches server in this session");

  std::cout << std::endl;
}

void DaemonCLI::DoStatus() {
  DaemonTools daemon;

  if (!daemon.IsRunning()) {
    std::cout << "Daemon status: not running" << std::endl;
    return;
  }

  std::cout << "Daemon status: running" << std::endl;
  PutDaemonParams(daemon.GetParams());

  Client client = MakeClient();
  auto rsp = client.Call<Client::Proc::GetStatus>({});

  std::cout << "Firmware status: " << rsp.ToCxxStr() << std::endl;

  client.SoftDisconnect();
}

void DaemonCLI::Execute(const TokenBuf& tokens) {
  DaemonCLI cli{tokens};
  cli.Run();
}

void DaemonCLI::Execute(int argc, char* argv[]) {
  if (!argv) throw FEXCEPT(std::runtime_error, "argv is NULL");

  TokenBuf tokens(argc, "");

  for (int i = 0; i < argc; ++i) {
    if (!argv[i])
      throw FEXCEPT(std::runtime_error,
                    "argv[" + std::to_string(i) + "] is NULL");

    tokens[i] = argv[i];
  }

  Execute(tokens);
}