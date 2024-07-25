#pragma once

#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "roki-mb-daemon/HwUtils.hpp"

namespace MbDaemon {
class FirmwareFSM {
 public:
  enum class FWState { Boot, Running, Transition, Fault };

  class FSMException : public std::runtime_error {
    static std::string ComposeMsg(std::string operation, FWState got,
                                  const std::vector<FWState> expected);

   public:
    FSMException(std::string operation, FWState got,
                 const std::vector<FWState> expected);

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
    static constexpr const char* Empty = "Nothing";
    static constexpr const char* ChipReset = "Chip reset";
    static constexpr const char* FWFlashing = "Firmware flashing";
    static constexpr const char* FWStarting = "Firmware starting";
    static constexpr const char* FaultTransition = "Fault Transition";
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

  std::string GetStateDescription() const;

  FWState GetState() const;
  std::string GetPort() const;

  FirmwareFSM();

  void ResetChip();
  void FlashBootlader(const std::string& firmware);
  void StartFirmware();
};

}  // namespace MbDaemon