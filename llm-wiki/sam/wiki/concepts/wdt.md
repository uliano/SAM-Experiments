---
title: WDT
type: concept
tags: [wdt, watchdog, firmware, samc21]
sources: [samc21-datasheet-ch23-wdt]
created: 2026-05-05
updated: 2026-05-05
---

# WDT

Watchdog Timer on the SAMC21. Generates a system reset if the application fails to
clear the counter within a configured timeout period. Clocked from OSCULP32K (~1 kHz).

## Overview

- Base address: 0x40002000
- Clock: CLK_WDT_OSC from OSCULP32K (always-on, ~1 kHz, not precise)
- Three modes: normal, window, always-on
- Reset values for CTRLA/CONFIG/EWCTRL loaded from NVM User Row

## Timeout Periods (CONFIG.PER)

| Value | Name | Period |
|-------|------|--------|
| 0x0 | CYC8 | ~8 ms |
| 0x4 | CYC128 | ~128 ms |
| 0x7 | CYC1024 | ~1 s |
| 0x8 | CYC2048 | ~2 s |
| 0x9 | CYC4096 | ~4 s |
| 0xA | CYC8192 | ~8 s |
| 0xB | CYC16384 | ~16 s |

## Enabling the WDT

```cpp
// 1. Enable APBA clock (usually enabled by default — WDT is on APBA)
// MCLK->APBAMASK.reg |= MCLK_APBAMASK_WDT;  // often already set

// 2. Configure timeout (while ENABLE=0)
WDT->CONFIG.reg = WDT_CONFIG_PER_CYC4096;   // ~4 s timeout

// 3. Enable
WDT->CTRLA.reg |= WDT_CTRLA_ENABLE;
while (WDT->SYNCBUSY.reg & WDT_SYNCBUSY_ENABLE);
```

## Clearing the Watchdog (Petting)

```cpp
while (WDT->SYNCBUSY.reg & WDT_SYNCBUSY_CLEAR);
WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;   // write 0xA5
```

Any value other than 0xA5 written to CLEAR causes an immediate reset.

## Window Mode

In window mode the counter has two thresholds:
- **WINDOW** (CONFIG[7:4]): defines the closed window — clearing too early (while window
  is still closed) triggers a reset.
- **PER** (CONFIG[3:0]): timeout — must clear before this.
- WINDOW value must be less than PER value.

```cpp
WDT->CTRLA.reg |= WDT_CTRLA_WEN;
while (WDT->SYNCBUSY.reg & WDT_SYNCBUSY_WEN);
WDT->CONFIG.reg = WDT_CONFIG_PER_CYC4096 | WDT_CONFIG_WINDOW_CYC2048;
```

## Always-On Mode

```cpp
WDT->CTRLA.reg |= WDT_CTRLA_ALWAYSON;
while (WDT->SYNCBUSY.reg & WDT_SYNCBUSY_ALWAYSON);
```

Once ALWAYSON is set it cannot be cleared by software. Only POR resets it. The WDT
cannot be disabled while ALWAYSON is set.

## Early Warning Interrupt

EWCTRL.EWOFFSET[3:0] fires the EW interrupt before the actual timeout, giving the
application a chance to react before reset. EWOFFSET must be less than PER.

```cpp
WDT->EWCTRL.reg = WDT_EWCTRL_EWOFFSET_CYC2048;
WDT->INTENSET.reg = WDT_INTENSET_EW;
NVIC_EnableIRQ(WDT_IRQn);
```

## Register Reference

| Offset | Name | Key bits |
|--------|------|---------|
| 0x00 | CTRLA | ALWAYSON(7)/WEN(2)/ENABLE(1) |
| 0x01 | CONFIG | WINDOW[7:4]/PER[3:0] — Write-Synchronized |
| 0x02 | EWCTRL | EWOFFSET[3:0] |
| 0x06 | INTFLAG | EW(0) — write-1-to-clear |
| 0x08 | SYNCBUSY | CLEAR(4)/ALWAYSON(3)/WEN(2)/ENABLE(1) |
| 0x0C | CLEAR | Write 0xA5 to pet; any other value = immediate reset |

## Key Facts

- CLK_WDT_OSC comes from OSCULP32K — ~1 kHz, not precise; timeout periods are approximate.
- NVM User Row sets factory defaults for CTRLA/CONFIG/EWCTRL on POR.
- CONFIG must be written while ENABLE=0 (except in always-on mode).
- SYNCBUSY must be polled before each CLEAR write — cannot write twice without waiting.
- Writing any value ≠ 0xA5 to CLEAR triggers an immediate system reset.

## See Also

- [[SAMC21 Datasheet Ch.23 WDT]] — register reference
- [[Clock System]] — OSCULP32K is always-on, feeds WDT
- [[Memory Map]] — WDT base address 0x40002000
- [[NVIC Interrupt Map]] — WDT IRQn
