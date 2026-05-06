# SAMC21 Bring-Up Base

Bare-metal ATSAMC21J18A-AU firmware base for board bring-up and future project
starts.

The active firmware is intentionally small: initialize the MCU from OSC48M,
bring up COM5 at 1000000 baud, then report external crystal status over serial.
It is meant for quick checks as new boards arrive from production.

## SAMC21J18A Peripheral Limitations — READ BEFORE DESIGNING

**Target device: ATSAMC21J18A-AU (J-variant, 64-pin). Not the N-variant.**

Critical differences from the N-variant that affect firmware design:

- **Timers**: TC0–TC4 only. **TC5, TC6, TC7 do not exist** (APB-D, N-variant only).
  TC4 is standalone — it cannot be paired with TC5 for 32-bit mode.
  Available ≥24-bit counters: TCC0 (24-bit), TCC1 (24-bit), TC0+TC1 (32-bit).
- **TCC widths**: TCC0=24-bit, TCC1=24-bit, TCC2=**16-bit** only.
- **CCL**: INSEL=ALT2TC (0xA) and INSEL=ASYNCEVENT (0xB) are N-series only (absent
  from SAMC21J18A CMSIS header). INSEL=TCC (0x8): datasheet marks it reserved on
  C20/C21, but CMSIS header defines it as "TCC input source" — likely functional,
  needs experimental verification. TCC→LUT mapping unknown.
- **AC clock errata (rev A only)**: GCLK_AC non-functional — but **annulled on
  our hardware**. Verified: DSU.DID=0x11010500 → Rev F (REVISION=0x5). All errata
  in DS80000740B were fixed by Rev E; Rev F is clean. Use GCLK_AC normally.
- **Port C**: absent on J-variant. Only PA (Port A) and PB (Port B) available.
- **GCLK IDs**: TCC0+TCC1 share PCHCTRL[**28**]; TCC2 = PCHCTRL[29];
  TC0+TC1 = PCHCTRL[30]; TC2+TC3 = PCHCTRL[31]; TC4 = PCHCTRL[32];
  CCL = PCHCTRL[38]; AC = PCHCTRL[34] (non-functional, use ADC1=[36]).
- **TC2+TC3**: errata §35.6.2.4 warns against use; avoid in new designs even
  though TC2 in COUNT32 mode has been observed to work experimentally.

See `llm-wiki/sam/wiki/concepts/ccl-configuration.md` and
`llm-wiki/sam/wiki/sources/samc21-errata.md` for full errata details.

---

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

## Language

Interact with the user in **Italian**. All written artefacts (code, comments,
documentation, wiki pages) are in **English**.

## Knowledge Base

A structured wiki for the SAMC21 lives in `llm-wiki/sam/wiki/`. Consult it
**first** whenever chip or project information is needed.

Priority order:

1. **`llm-wiki/sam/wiki/`** — primary source. Contains concepts, entities,
   datasheet chapter summaries, and experimentally verified errata notes.
2. **`llm-wiki/sam/raw/`** and **`docs/`** (datasheet PDF) — last resort.
   Use only when the wiki contains a controversy or is missing information.
   In the latter case, run a wiki lint (`/lint` in the `llm-wiki/` directory)
   **at the same time** to back-fill the gap, so the raw source never needs
   to be consulted twice for the same topic.

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
