#include "FirmwareFSM.hpp"
using namespace Roki;

std::string FirmwareFSM::FSMException::ComposeMsg(
    std::string operation, FWState got, const std::vector<FWState> expected) {
  std::string msg =
      "Inappropriate state for operation \"" + operation + "\": " + "\n";
  msg += "  Got " + std::string(StateToStr(got)) + ",\n";

  msg += "  Expected one of: ";

  for (int i = 0; i < expected.size() - 1; ++i) {
    msg += std::string(StateToStr(expected[i])) + ", ";
  }

  msg += StateToStr(expected.back());
}

FirmwareFSM::FSMException::FSMException(std::string operation, FWState got,
                                        const std::vector<FWState> expected)
    : std::runtime_error{ComposeMsg(operation, got, expected)} {}

void FirmwareFSM::SetState(FWState state) {
  std::lock_guard _{*Mutex};
  State = state;
  RoutineName = Routines::Empty;
}

bool FirmwareFSM::CheckState(const std::vector<FWState>& expected) {
  bool passed = false;
  for (auto required : expected) passed = passed ? true : State == required;
  return passed;
}

void FirmwareFSM::DoTransition(TransitionRoutine routine) {
  Mutex->lock();
  bool stateOk = CheckState(routine.ExpectedStates);
  if (stateOk) {
    State = FWState::Transition;
    RoutineName = routine.Name;
  }
  Mutex->unlock();

  if (!stateOk) throw FSMException(routine.Name, State, routine.ExpectedStates);

  FWState newState = FWState::Fault;
  Helpers::Defer _{[this, &newState] { SetState(newState); }};

  routine.Action();
  newState = routine.FinalState;
}

const char* FirmwareFSM::StateToStr(FWState state) {
  switch (state) {
    case FWState::Boot:
      return "Boot";

    case FWState::Running:
      return "Running";

    case FWState::Fault:
      return "Fault";

    case FWState::Transition:
      return "Transition";

    default:
      return "Bad FWState";
  }
}

auto FirmwareFSM::GetState() const -> FWState {
  std::lock_guard _{*Mutex};
  return State;
}

std::string FirmwareFSM::GetPort() const {
  std::lock_guard _{*Mutex};
  return MbPort;
}

FirmwareFSM::FirmwareFSM()
    : State{FWState::Boot}, Mutex{std::make_unique<std::mutex>()} {
  ResetChip();
  MbPort = HwUtils::BootloaderFind();
}

void FirmwareFSM::ResetChip() {
  TransitionRoutine routine;

  routine.Name = Routines::ChipReset;
  routine.Action = HwUtils::ChipReset;
  routine.ExpectedStates = {FWState::Boot, FWState::Running, FWState::Fault};
  routine.FinalState = FWState::Boot;

  DoTransition(routine);
}

void FirmwareFSM::FlashBootlader(const std::string& firmware) {
  TransitionRoutine routine;

  routine.Name = Routines::FWFlashing;
  routine.Action = [&firmware] { HwUtils::BootloaderFlash(firmware); };
  routine.ExpectedStates = {FWState::Boot};
  routine.FinalState = FWState::Boot;

  DoTransition(routine);
}

void FirmwareFSM::StartFirmware() {
  TransitionRoutine routine{};

  routine.Name = Routines::FWStarting;
  routine.Action = HwUtils::BootloaderStart;
  routine.ExpectedStates = {FWState::Boot};
  routine.FinalState = FWState::Running;

  DoTransition(routine);
}