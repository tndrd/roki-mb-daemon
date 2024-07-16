#include "Server.hpp"

using namespace Roki;
using namespace Helpers;

template <typename Proc>
typename Proc::Responce Server::HandlerImpl(
    const typename Proc::Request& request) {
  std::cout << RPCDefs::ProcIDs::ToStr(Proc::ID) << std::endl;
}