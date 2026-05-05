# SAMC21 Bring-Up Base

Bare-metal PlatformIO base for ATSAMC21J18A-AU projects.

The default firmware starts COM5 at 1000000 baud and reports board-clock status:

- CPU clock from OSC48M/GCLK0 at 48 MHz
- 24 MHz external crystal on PA14/PA15, including clock-fail detection
- optional compile-time 32.768 kHz external crystal test on PA00/PA01, including clock-fail detection

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
CPU clock: OSC48M/GCLK0 OK 48000000 Hz
XOSC 24 MHz PA14/PA15: OK ready_wait_ms=... ready=yes fail_detected=no status=... gclk1=enabled gclk2=enabled
XOSC32K PA00/PA01: SKIP build_flag=SAM_BOARD_TEST_XOSC32K=0
```

Set `SAM_BOARD_TEST_XOSC32K=1` in `platformio.ini` only for boards populated
with the optional 32.768 kHz crystal. With that flag enabled, the XOSC32K line
reports `OK`/`FAIL`, `ready_wait_ms`, `ready`, `fail_detected`, status, and
`gclk3`.

## Hardware Notes

- PB30/PB31 are reserved for UART to USB.
- PA14/PA15 are reserved for the 24 MHz crystal.
- PA00/PA01 are reserved for the 32.768 kHz crystal footprint.
- PA03 and PA04 are VREF conditioning pins with 22 ohm series resistance and
  22 nF to ground; treat them as capacitively loaded analog/reference pins.
