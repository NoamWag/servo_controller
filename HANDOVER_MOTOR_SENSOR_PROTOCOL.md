# Next session plan

Python-side parsing for the current sensor response is already handled on the host side, so this document is intentionally short.

## Next milestone

Next session, we will design a self-calibration mechanism for the motor control loop.

Goals:
- measure actual motor speed versus commanded speed
- build a calibration table or model for each motor
- estimate the relationship between command argument and real RPM / velocity
- close the control loop around the motor based on measured output

## Planned work

1. Add a calibration mode that runs the motor at a known set of command values.
2. Capture timing data from the GP22 edge sensor while the motor is running.
3. Convert the captured edge times into speed estimates.
4. Fit the measured speed vs. command relationship.
5. Use the learned model to close the control loop and correct command values dynamically.

## Notes

- This is the next firmware and control-design step after the UART sensor query path is working.
- The host side will continue to interpret the raw sensor samples and feed the calibration logic.
- The calibration will be the bridge from raw motor command output to closed-loop control.
