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
- PA04 is suspect/damaged on the current bench board. Do not use it in generic
  bring-up firmware; keep PA04-specific tests isolated.

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
- `upload_protocol = atmel-ice`
- `debug_tool = atmel-ice`

## Source Map

```text
src/
  main.cpp          - boot, timebase, serial, quartz test loop
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
The external crystals are started by `QuartzTest` with timeouts, so a missing or
bad crystal cannot trap boot before serial output.

The serial report repeats every 2 seconds:

```text
SAMC21 board bring-up
UART: SERCOM5 PB30/PB31, 1000000 baud
OSC48M/GCLK0: OK 48000000 Hz
XOSC 24 MHz PA14/PA15: OK/FAIL ...
XOSC32K PA00/PA01: OK/FAIL ...
```

If the 32.768 kHz crystal is not populated, an `XOSC32K FAIL` report is expected
and is not a boot failure.

## Next PA04 Work

After this baseline, add PA04 tests as an explicit diagnostic mode. Suggested
order:

1. Passive GPIO input test with external low/high and pull disabled.
2. Internal pull-up/pull-down observation if safe for the board state.
3. Weak output drive through a series resistor, measured externally.
4. ADC0/AIN4 and AC/AIN0 only after GPIO behavior is understood.
