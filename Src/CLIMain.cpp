#include "roki-mb-daemon/CLI.hpp"

int main(int argc, char* argv[]) try {
  MbDaemon::DaemonCLI::Execute(argc, argv);
} catch (...) {
}
