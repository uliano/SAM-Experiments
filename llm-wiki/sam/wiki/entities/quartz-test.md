---
title: QuartzTest
type: entity
tags: [firmware, crystal, xosc, xosc32k, bring-up]
sources: [application-bring-up]
created: 2026-05-05
updated: 2026-05-05
---

# QuartzTest

Crystal startup verification and diagnostic reporter. Defined in `src/quartz_test.hpp`.

## Purpose

Starts the external 24 MHz crystal (XOSC) with a timeout so boot never hangs on a missing
or faulty crystal. Optionally starts the 32.768 kHz crystal (XOSC32K). Reports status to Serial.

## Compile-Time Configuration

```cpp
#define SAM_BOARD_TEST_XOSC32K 0   // default: skip XOSC32K test
```

Set to `1` in `platformio.ini` only on boards populated with the optional 32 kHz crystal.

## API

```cpp
QuartzTest::init();         // start crystals, wait for ready or timeout
QuartzTest::report_once();  // print one status report then return
QuartzTest::run_forever();  // print report every 2 s, never returns
```

## XOSC (24 MHz) Configuration

```cpp
OSCCTRL->XOSCCTRL.reg =
    OSCCTRL_XOSCCTRL_STARTUP(8)  |  // startup time ~16k cycles
    OSCCTRL_XOSCCTRL_AMPGC       |  // automatic gain control
    OSCCTRL_XOSCCTRL_GAIN(4)     |  // gain for > 8 MHz crystal
    OSCCTRL_XOSCCTRL_CFDEN       |  // crystal failure detection enabled
    OSCCTRL_XOSCCTRL_XTALEN      |  // crystal mode (not CMOS clock)
    OSCCTRL_XOSCCTRL_ENABLE;
```

Timeout: 100 ms. After timeout, boot continues regardless.

## XOSC32K Configuration (if enabled)

```cpp
OSC32KCTRL->XOSC32K.reg =
    OSC32KCTRL_XOSC32K_STARTUP(7)  |  // startup ~32k cycles (~1 s at 32 kHz)
    OSC32KCTRL_XOSC32K_EN32K       |
    OSC32KCTRL_XOSC32K_XTALEN      |
    OSC32KCTRL_XOSC32K_ENABLE;
```

Crystal failure detection also enabled (`OSC32KCTRL->CFDCTRL`).
Timeout: 6000 ms (32 kHz crystals can take several seconds to start).

## GCLK Generators Configured on Success

| Generator | Source | Frequency | Condition |
|-----------|--------|-----------|-----------|
| GCLK1 | XOSC | 24 MHz | XOSC ready and not failed |
| GCLK2 | XOSC / 64 | 375 kHz | XOSC ready and not failed |
| GCLK3 | XOSC32K | 32.768 kHz | XOSC32K ready (build flag must be 1) |

Generators are configured lazily (`xosc_generators_configured_` flag) — only once.
`refresh_xosc_generators()` is idempotent.

## Serial Report Format

```
SAMC21 board bring-up
UART: SERCOM5 PB30/PB31, 1000000 baud
CPU clock: OSC48M/GCLK0 OK 48000000 Hz
XOSC 24 MHz PA14/PA15: OK ready_wait_ms=3 ready=yes fail_detected=no status=0x00000002 gclk1=enabled gclk2=enabled
XOSC32K PA00/PA01: SKIP build_flag=SAM_BOARD_TEST_XOSC32K=0
```

## Crystal Failure Detection

`xosc_failed()` checks both `OSCCTRL->STATUS.XOSCFAIL` and `OSCCTRL->INTFLAG.XOSCFAIL`.
`xosc_active()` requires `ready && !failed`.

XOSC failure detection uses the Clock Failure Detection (CFD) circuit in OSCCTRL. When
enabled (`CFDEN`), a glitch or oscillation stop sets `XOSCFAIL`. The CFD prescaler is set
to 0 (`CFDPRESC = 0`).

## See Also

- [[Clock System]] — full clock configuration context
- [[Application Bring-Up Source]] — boot sequence
- [[Timebase]] — `wait_for_xosc_ms` uses `Timebase::millis()`
