# Servo Controller Project Summary

## Overview

This project is a Raspberry Pi Pico firmware controller for a dual-motor servo-style drive system. The firmware uses UART command handling as the host interface and a motor state manager to drive PWM-controlled motor outputs through a simple L293D-style driver arrangement.

## Current state

### UART communication layer

The project includes a UART command manager that decodes incoming commands from the host.

Supported opcodes:

- `0`    - `kGetVer`
- `1`    - `kGetTime`
- `257`  - `kSetTime`
- `258`  - `kEcho`
- `259`  - `kStartMotor`
- `260`  - `kStopMotor`
- `261`  - `kGetMotorSensor` (currently supports motor `0`; reads the GP22 edge-capture history and returns a compact binary payload)

The host protocol is little-endian and follows a simple structure:

- `uint16_t op`
- payload bytes immediately following

The command manager processes incoming data and sends acknowledgements, text responses, and compact binary sensor replies as needed.

For `kGetMotorSensor`, the host sends:

- `uint16_t op = 261`
- `uint8_t motor`

The Pico replies with:

- `uint16_t op = 2` (`kMotorSensor`)
- `uint8_t count`
- a packed sensor payload containing up to `MOTOR_CONTROLLER_HISTORY_SIZE` timestamps in newest-to-oldest order
- packed direction bits for each transition (`1` = rising, `0` = falling)

The response is intentionally compact and fixed-size so it fits within the 50-byte UART frame limit. The implementation stores the count first, then up to 10 `uint32_t` timestamps, followed by the packed direction bits.

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

### GP22 sensor edge capture

The firmware also includes a GP22 input capture module for edge timing on the sensor line. This is currently the only implemented sensor input and is associated with motor index `0`.

- Sensor GPIO: GP22
- Motor mapping for now: motor `0` -> GP22 sensor
- Motor index `1` is not yet mapped to a sensor input
- Motor index `2` is not defined / TBD and is intentionally not supported in the current protocol

The sensor stores a 10-sample circular buffer containing:

- the DSP timestamp of each rising/falling transition
- the direction of the edge (`1` = rising, `0` = falling)
- the current write index
- the number of filled entries

This module is implemented in `motor_controller.h` and `motor_controller.c`, and the host can query it via `kGetMotorSensor`.

## GPIO assignment

UART pins remain reserved for communication:

- UART TX: GP4
- UART RX: GP5

Sensor input pins:

- GP22: sensor edge capture input for motor index `0` (currently implemented)
- Motor index `1` sensor input: not yet assigned
- Motor index `2`: TBD / not yet defined

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
- `motor_controller.h` - GP22 edge-capture history API
- `motor_controller.c` - GP22 GPIO interrupt capture and circular buffer logic
- `CMakeLists.txt` - Pico SDK build configuration
- `HANDOVER_MOTOR_SENSOR_PROTOCOL.md` - host-side protocol handover for the sensor query command
- `.github/copilot-instructions.md` - host-side UART generation guidance
