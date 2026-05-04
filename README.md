# SAMC21 Bring-Up Base

Bare-metal PlatformIO base for ATSAMC21J18A-AU projects.

The default firmware starts COM5 at 1000000 baud and reports board-clock status:

- OSC48M/GCLK0 at 48 MHz
- 24 MHz external crystal on PA14/PA15
- optional 32.768 kHz external crystal on PA00/PA01

External crystal checks use timeouts, so serial output still appears on boards
with a missing or faulty crystal.

## Build And Run

```text
pio run
pio run -t upload
pio device monitor -p COM5 -b 1000000
```

## Expected Serial Report

```text
SAMC21 board bring-up
UART: SERCOM5 PB30/PB31, 1000000 baud
OSC48M/GCLK0: OK 48000000 Hz
XOSC 24 MHz PA14/PA15: OK ready_wait_ms=... status=... gclk1=OK gclk2=OK
XOSC32K PA00/PA01: OK ready_wait_ms=... status=... gclk3=OK
```

If the 32.768 kHz crystal is not populated, `XOSC32K FAIL` is expected.

## Hardware Notes

- PB30/PB31 are reserved for UART to USB.
- PA14/PA15 are reserved for the 24 MHz crystal.
- PA00/PA01 are reserved for the 32.768 kHz crystal footprint.
- PA04 is suspect on the current bench board. Keep PA04 tests isolated from the
  generic bring-up firmware.
