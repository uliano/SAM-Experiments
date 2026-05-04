# SAMC21 Bring-Up Base - Project Context

## What This Is

Bare-metal firmware base for the ATSAMC21J18A-AU custom proto-board.

This repository is the starting point for new SAMC21 projects and for incoming
production-board checks. It keeps the custom PlatformIO build system, CMSIS
device headers, linker script, startup code, UART/time utilities, and a simple
serial bring-up firmware.

The active firmware tests board clocks through COM5:

- internal OSC48M configured as GCLK0 at 48 MHz
- external 24 MHz crystal on PA14/PA15
- external 32.768 kHz crystal footprint on PA00/PA01

The external crystals are tested with timeouts. Boot must not hang if a crystal
is missing, unpopulated, or faulty.

## Hardware - Custom Proto Board

| Resource | Details |
|---|---|
| MCU | ATSAMC21J18A-AU, 64-pin |
| Crystal 1 | 24 MHz on PA14/PA15 |
| Crystal 2 | 32.768 kHz footprint on PA00/PA01 |
| UART to USB | PB30 TX, PB31 RX, COM5, 1000000 baud |
| Debug | Atmel-ICE via SWD |
| Bench LED | PB23 active high, jumperable |
| Bench button | PB22 active high with external pulldown, jumperable |

## Pin Notes

- PA14/PA15 are reserved for the 24 MHz crystal.
- PA00/PA01 are reserved for the optional 32.768 kHz crystal footprint.
- PB30/PB31 are reserved for the UART to USB bridge.
- PA03 and PA04 are used on this board as VREF conditioning pins. Each is tied
  through a 22 ohm resistor and has 22 nF to ground to smooth the VREF node.
  Treat them as capacitively loaded analog/reference pins, not bare fast GPIOs.
- LEDs and buttons are jumperable test fixtures. `pins.hpp` records current
  bench wiring, not permanent board constraints.

## Build System

PlatformIO, no Arduino, no ASF, no HAL.

Important settings live in `platformio.ini`:

```ini
board = samc21_xpro
board_build.mcu = samc21j18a
board_build.f_cpu = 48000000L
board_build.ldscript = linker/samc21j18.ld
build_flags = -std=gnu++23 -D__SAMC21J18A__ -DDONT_USE_CMSIS_INIT ...
upload_protocol = atmel-ice
debug_tool = atmel-ice
monitor_port = COM5
monitor_speed = 1000000
```

Toolchain path currently points at the local ARM GNU toolchain in
`C:/Users/uliano/stuff/toolchains/arm-gnu-toolchain-15.2.rel1-mingw-w64-i686-arm-none-eabi`.

## Source Layout

```text
src/
  main.cpp          - boot, timebase, serial, one-shot quartz test report
  init.hpp          - minimal OSC48M/GCLK0 setup
  quartz_test.hpp   - serial report for 24 MHz and 32.768 kHz crystals
  pins.hpp          - UART pins and current bench fixtures
  serial.hpp/.cpp   - global Serial instance on SERCOM5
  interrupts.cpp    - SERCOM5 and SysTick IRQ hooks
  core/             - reusable bare-metal utility layer
```

Useful core utilities:

- `Pin<Bank, N>` zero-overhead GPIO abstraction
- `RingBuffer<T, N>` power-of-two FIFO where `N` is the exponent
- `Uart<Interrupt,...>` interrupt-driven UART
- `SerialPort` print wrapper
- `ByteStream` `std::to_chars` number formatting
- `Timebase` SysTick millis/micros
- custom `startup_samc21.c` vector table/reset handler
- `syscalls.cpp` newlib I/O bridge to `Serial`

## Active Firmware Behavior

After reset:

1. `sys_init()` sets flash wait states, OSC48M full speed, and GCLK0.
2. `Timebase::init()` starts SysTick at 1 kHz.
3. `Serial.init(1000000)` starts SERCOM5 on PB30/PB31.
4. `QuartzTest` starts the 24 MHz external crystal with a timeout.
5. If `SAM_BOARD_TEST_XOSC32K=1`, `QuartzTest` also starts and checks the
   optional 32.768 kHz crystal with a timeout. The default is `0` for fast boot
   on boards without the optional crystal.
6. A single report is printed after reset.

Expected serial shape:

```text
SAMC21 board bring-up
UART: SERCOM5 PB30/PB31, 1000000 baud
OSC48M/GCLK0: OK 48000000 Hz
XOSC 24 MHz PA14/PA15: OK ready_wait_ms=... status=... gclk1=OK gclk2=OK
XOSC32K PA00/PA01: SKIP build_flag=SAM_BOARD_TEST_XOSC32K=0
```

When `SAM_BOARD_TEST_XOSC32K=1`, the XOSC32K line reports `OK`/`FAIL`,
`ready_wait_ms`, status, and `gclk3`. Use that flag only on boards populated
with the optional 32.768 kHz crystal.

## Conventions

- Direct register access through CMSIS headers.
- Static struct pattern for small peripheral test modules.
- Templates over inheritance for zero-cost hardware abstractions.
- C++23 is available.
- No dynamic allocation, exceptions, or RTTI.
- Keep generic bring-up code free of board-specific experiments unless they are
  clearly isolated.

## PA03/PA04 VREF Note

PA04 was checked as GPIO, AC input, and ADC input and behaved correctly. Earlier
PA04 confusion was likely caused by the local 22 nF capacitor slowing fast pin
transitions. Keep PA03/PA04 diagnostics out of generic bring-up firmware unless
they are deliberately reintroduced as isolated tests.
