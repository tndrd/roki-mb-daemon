# roki-mb-daemon
 
A Linux daemon that manages hardware access to the microcontrollers of the
**Roki** humanoid robot (Starkit RoboCup team). It runs on the robot's
Raspberry Pi and mediates every operation on the two STM32 controllers wired to
the board (firmware start, flashing, reset and status).

## Stack

C++ · Linux · TCP sockets · pthread · pigpio · CMake
 
## Why it exists
 
Robot bring-up used to be a pile of loose Python scripts: one to start firmware,
one to flash, one to reset, each launched by hand and racing each other for the
serial port. Ports occasionally swapped between devices, and nothing reported the
state of the hardware. This daemon replaces that with one authority: all hardware
operations go through it, in a defined order, with diagnostics and logs. Even the
host-side control library asks the daemon for permission before it may occupy a
port.
 
## What it does
 
- **Networked control service** - a TCP server exposes hardware operations over a
  typed RPC protocol, so any process (local or remote) drives the controllers
  through one well-defined interface instead of ad-hoc scripts.
- **Port arbitration** - the daemon hands a serial port to exactly one client at a
  time, eliminating races for the resource by design.
- **Firmware lifecycle** - flash, start, reset and status of the STM32
  controllers, including driving the bootloader pins over GPIO.
- **State machine** - each controller's firmware state (Boot / Running /
  Transition / Fault) is modelled explicitly; an operation requested from an
  illegal state is rejected rather than run blindly.
- **Diagnostics & CLI** - logging plus a command-line client for manual
  inspection and control.
