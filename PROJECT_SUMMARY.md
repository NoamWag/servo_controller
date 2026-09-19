# Servo Controller Project Summary

## Overview

This project is a Raspberry Pi Pico firmware controller for a dual-motor servo-style drive system. The firmware uses UART command handling as the host interface and a motor state manager to drive PWM-controlled motor outputs through a simple L293D-style driver arrangement.

## Current state

### UART communication layer

The project includes a UART command manager that decodes incoming commands from the host.

Supported opcodes:

- `0`  - `kGetVer`
- `1`  - `kGetTime`
- `257` - `kSetTime`
- `258` - `kEcho`
- `259` - `kStartMotor`
- `260` - `kStopMotor`

The host protocol is little-endian and follows a simple structure:

- `uint16_t op`
- payload bytes immediately following

The command manager processes incoming data and sends acknowledgements or text responses as needed.

### Time handling

The firmware supports time state tracking with a reference clock and elapsed time calculations based on microseconds. This allows the Pico to maintain a logical DSP time and answer `GetTime` requests from the host.

### Motor management

The project now includes a motor manager state machine with the following behavior:

- `start_motor(motor, direction, speed)`
- `stop_motor(motor)`
- state transitions between idle and in-motion
- direction and speed updates while a motor is already active
- GPIO and PWM output handling for each motor

The motor manager owns the hardware state, while the communication layer triggers external commands to it.

## GPIO assignment

UART pins remain reserved for communication:

- UART TX: GP4
- UART RX: GP5

Motor pins are configured as:

- Motor 1 PWM: GP16
- Motor 1 IN1: GP17
- Motor 1 IN2: GP18
- Motor 2 PWM: GP19
- Motor 2 IN3: GP20
- Motor 2 IN4: GP21

These are set up for a basic L293D-style motor driver with one PWM enable per motor and two direction pins per motor.

## Build status

The firmware builds successfully with the Pico SDK and produces output artifacts in the `build/` directory, including:

- `servo_controller.elf`
- `servo_controller.bin`
- `servo_controller.uf2`

## Next work areas

Planned extensions for the next stage include:

1. adding higher-level motor control APIs, such as brake, ramp, or acceleration helpers
2. refining the motor state machine with more explicit command states and transitions
3. adding validation and safety checks for invalid motor IDs, direction values, and speed ranges
4. preparing a more advanced host-side control layer for high-level motion commands
5. continuing hardware validation once a programming/debugging device is available

## Files in the project

- `servo_controller.c` - main firmware entry point
- `comm_manager.h` - UART protocol definitions
- `comm_manager.c` - UART receive/state processing logic
- `motor_manager.h` - public motor API
- `motor_manager.c` - state-machine-driven motor control implementation
- `CMakeLists.txt` - Pico SDK build configuration
- `.github/copilot-instructions.md` - host-side UART generation guidance
