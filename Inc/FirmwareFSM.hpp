#pragma once

#include <thread>

#include "HwUtils.hpp"

namespace Roki {
class FirmwareFSM {
 public:
  enum class FWState { Boot, Running, Transition, Fault };

  class FSMException : public std::runtime_error {
    static std::string ComposeMsg(std::string operation, FWState got,
                                  const std::vector<FWState> expected);

   public:
    FSMException(std::string operation, FWState got,
                 const std::vector<FWState> expected)
        : std::runtime_error{ComposeMsg(operation, got, expected)} {}

    virtual ~FSMException() = default;
  };

 private:
  struct TransitionRoutine {
    std::string Name;
    std::function<void()> Action;

    std::vector<FWState> ExpectedStates;
    FWState FinalState;
  };

 private:
  struct Routines {
    static constexpr char* Empty = "Nothing";
    static constexpr char* ChipReset = "Chip reset";
    static constexpr char* FWFlashing = "Firmware flashing";
    static constexpr char* FWStarting = "Firmware starting";
    static constexpr char* FaultTransition = "Fault Transition";
  };

  FWState State;
  mutable std::unique_ptr<std::mutex> Mutex;
  std::string RoutineName = Routines::Empty;
  std::string MbPort;

 private:
  void SetState(FWState state);
  bool CheckState(const std::vector<FWState>& expected);
  void DoTransition(TransitionRoutine routine);

 public:
  static const char* StateToStr(FWState state);

  FWState GetState() const;
  std::string GetPort() const;

  FirmwareFSM();

  void ResetChip();
  void FlashBootlader(const std::string& firmware);
  void StartFirmware();
};

}  // namespace Roki