# Host UART motor command instructions

When generating host-side code for this project, follow the protocol exactly as implemented on the Pico firmware.

## Packet format

All UART messages use a little-endian 16-bit operation code followed by a payload.

- Message layout: `uint16_t op` + payload bytes
- Maximum message size: 50 bytes total
- The firmware expects the `op` field at the start of the packet and reads it as a little-endian uint16 value.

## Operation codes

- `kGetVer` = 0
- `kGetTime` = 1
- `kSetTime` = 257
- `kEcho` = 258
- `kStartMotor` = 259
- `kStopMotor` = 260

## Motor command payloads

### Start motor

Host packet:

- `op = 259`
- payload:
  - `motor: uint8_t`
  - `direction: uint8_t` where `0 = forward`, `1 = reverse`
  - `speed: uint16_t` little-endian, valid range 0..1000

Example byte layout:

```text
[op_lo, op_hi, motor, direction, speed_lo, speed_hi]
```

Equivalent C-like struct:

```c
typedef struct {
    uint8_t motor;
    uint8_t direction;
    uint16_t speed;
} tPcToDspMessageStartMotor;
```

### Stop motor

Host packet:

- `op = 260`
- payload:
  - `motor: uint8_t`

Example byte layout:

```text
[op_lo, op_hi, motor]
```

Equivalent C-like struct:

```c
typedef struct {
    uint8_t motor;
} tPcToDspMessageStopMotor;
```

## Host-side implementation guidance

When writing host code:

1. Build a buffer with `uint8_t[]`.
2. Write `op` as little-endian two bytes.
3. Write the payload bytes in the defined order.
4. Send the full buffer over UART.
5. Read the response and interpret `op` in the same way.
6. For motor commands, expect an ACK reply with `op = 0`.

## Example Python host helper

```python
import struct

OP_START_MOTOR = 259
OP_STOP_MOTOR = 260


def send_start_motor(port, motor: int, direction: int, speed: int):
    if not 0 <= motor <= 1:
        raise ValueError("motor must be 0 or 1")
    if direction not in (0, 1):
        raise ValueError("direction must be 0 (forward) or 1 (reverse)")
    if not 0 <= speed <= 1000:
        raise ValueError("speed must be 0..1000")

    payload = struct.pack('<BBH', motor, direction, speed)
    packet = struct.pack('<H', OP_START_MOTOR) + payload
    port.write(packet)


def send_stop_motor(port, motor: int):
    if not 0 <= motor <= 1:
        raise ValueError("motor must be 0 or 1")

    packet = struct.pack('<H', OP_STOP_MOTOR) + struct.pack('<B', motor)
    port.write(packet)
```

## Important notes

- Use little-endian encoding for all multi-byte values.
- Keep the packet compact and avoid extra padding.
- The firmware is written to accept the exact field order above.
- Do not add extra bytes before the `op` field.
- `start_motor` should be sent as a full command; do not fragment it across separate writes unless the transport layer explicitly does framing.

## Preferred behavior for generated code

When scaffolding host code, generate helpers named like:

- `send_start_motor(...)`
- `send_stop_motor(...)`
- `encode_start_motor(...)`
- `encode_stop_motor(...)`

These helpers should build the raw UART packet and not depend on the firmware state machine.
