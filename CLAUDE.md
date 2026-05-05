# SAMC21 Bring-Up Base

Bare-metal ATSAMC21J18A-AU firmware base for board bring-up and future project
starts.

The active firmware is intentionally small: initialize the MCU from OSC48M,
bring up COM5 at 1000000 baud, then report external crystal status over serial.
It is meant for quick checks as new boards arrive from production.

## Current Hardware Assumptions

- MCU: ATSAMC21J18A-AU.
- 24 MHz crystal: PA14/PA15.
- Optional 32.768 kHz crystal footprint: PA00/PA01.
- UART to USB: PB30 TX, PB31 RX, COM5, 1000000 baud.
- Debug: Atmel-ICE via SWD.
- Bench LED: PB23, active high.
- Bench button: PB22, active high with external pulldown.
- PA03 and PA04 are VREF conditioning pins with 22 ohm series resistance and
  22 nF to ground; treat them as capacitively loaded analog/reference pins.

## Build

PlatformIO, no Arduino, no ASF, no HAL.

```text
pio run
pio run -t upload
pio device monitor -p COM5 -b 1000000
```

Key settings are in `platformio.ini`:

- `board_build.mcu = samc21j18a`
- `board_build.f_cpu = 48000000L`
- `board_build.ldscript = linker/samc21j18.ld`
- `build_flags = -std=gnu++23 -D__SAMC21J18A__ -DDONT_USE_CMSIS_INIT ...`
- `SAM_BOARD_TEST_XOSC32K=0` by default; set to `1` only on boards populated
  with the optional 32.768 kHz crystal.
- `upload_protocol = atmel-ice`
- `debug_tool = atmel-ice`

## Source Map

```text
src/
  main.cpp          - boot, timebase, serial, one-shot quartz test report
  init.hpp          - minimal OSC48M/GCLK0 setup
  quartz_test.hpp   - serial report for XOSC and XOSC32K
  pins.hpp          - UART and bench fixture pin aliases
  serial.hpp/.cpp   - global Serial instance
  interrupts.cpp    - SysTick and SERCOM5 handlers
  core/             - reusable bare-metal utilities
```

Core utilities worth preserving:

- `Pin<Bank, N>`
- `RingBuffer<T, N>` where `N` is the exponent
- `Uart<Interrupt,...>`
- `SerialPort`
- `ByteStream` printing through `std::to_chars`
- `Timebase`
- custom startup/vector table
- newlib syscalls to Serial

## Active Firmware

`sys_init()` only configures flash wait states, OSC48M full speed, and GCLK0.
`QuartzTest` starts the 24 MHz external crystal with a timeout. The optional
32.768 kHz crystal test is compile-time controlled by `SAM_BOARD_TEST_XOSC32K`.

The serial report is printed once after reset:

```text
SAMC21 board bring-up
UART: SERCOM5 PB30/PB31, 1000000 baud
CPU clock: OSC48M/GCLK0 OK 48000000 Hz
XOSC 24 MHz PA14/PA15: OK/FAIL ...
XOSC32K PA00/PA01: SKIP build_flag=SAM_BOARD_TEST_XOSC32K=0
```

When `SAM_BOARD_TEST_XOSC32K=1`, the XOSC32K line reports `OK`/`FAIL`,
`ready_wait_ms`, `ready`, `fail_detected`, status, and `gclk3`.
