---
title: SAMC21 Datasheet Chapter 23 — WDT
type: source
tags: [wdt, watchdog, samc21, datasheet]
date: 2026-05-05
---

# SAMC21 Datasheet Ch. 23 — WDT – Watchdog Timer

Source: SAM C20/C21 Data Sheet DS60001479D, chapter 23, pages 264–278.

## Overview

The Watchdog Timer (WDT) generates a system reset if the application does not clear
the watchdog counter within a defined time window. The WDT clock is CLK_WDT_OSC
derived from OSCULP32K (~1 kHz, not precise). Three operating modes: normal, window,
and always-on. Reset values for CTRLA, CONFIG, and EWCTRL are loaded from the NVM
User Row on power-on reset.

## Base Address

0x40002000

## Operating Modes

| Mode | Description |
|------|-------------|
| Normal | Clear the counter at any time within the timeout period |
| Window | Two-stage: must NOT clear too early (before window opens) and must clear before timeout |
| Always-On | CTRLA.ALWAYSON=1; cannot be disabled except by POR |

## Timeout / Window Periods (CONFIG.PER, CONFIG.WINDOW)

| Value | Name | Approximate period |
|-------|------|--------------------|
| 0x0 | CYC8 | 8 cycles ~8 ms |
| 0x1 | CYC16 | 16 cycles ~16 ms |
| 0x2 | CYC32 | 32 cycles ~32 ms |
| 0x3 | CYC64 | 64 cycles ~64 ms |
| 0x4 | CYC128 | 128 cycles ~128 ms |
| 0x5 | CYC256 | 256 cycles ~256 ms |
| 0x6 | CYC512 | 512 cycles ~512 ms |
| 0x7 | CYC1024 | 1024 cycles ~1 s |
| 0x8 | CYC2048 | 2048 cycles ~2 s |
| 0x9 | CYC4096 | 4096 cycles ~4 s |
| 0xA | CYC8192 | 8192 cycles ~8 s |
| 0xB | CYC16384 | 16384 cycles ~16 s |

## Register Map (base 0x40002000)

| Offset | Name | Reset | Description |
|--------|------|-------|-------------|
| 0x00 | CTRLA | (NVM) | ALWAYSON(7)/WEN(2)/ENABLE(1) |
| 0x01 | CONFIG | (NVM) | WINDOW[7:4]/PER[3:0] — Write-Synchronized |
| 0x02 | EWCTRL | (NVM) | EWOFFSET[3:0] — early warning interrupt offset |
| 0x04 | INTENCLR | 0x00 | EW — early warning interrupt enable clear |
| 0x05 | INTENSET | 0x00 | EW — early warning interrupt enable set |
| 0x06 | INTFLAG | 0x00 | EW — early warning interrupt flag |
| 0x08 | SYNCBUSY | 0x00 | CLEAR(4)/ALWAYSON(3)/WEN(2)/ENABLE(1) |
| 0x0C | CLEAR | 0xFF | Write 0xA5 to clear; any other value = immediate reset |

## CTRLA Key Bits

| Bit | Name | Description |
|-----|------|-------------|
| 7 | ALWAYSON | Once set, cannot be cleared except by POR; takes precedence over ENABLE |
| 2 | WEN | Window mode enable |
| 1 | ENABLE | Enable watchdog; write-synchronized |

## CONFIG Register

| Bits | Name | Description |
|------|------|-------------|
| 7:4 | WINDOW[3:0] | Window close timeout (window mode only) |
| 3:0 | PER[3:0] | Timeout period (see table above) |

Write-Synchronized: wait SYNCBUSY=0 before modifying CONFIG.

## CLEAR Register

Write-only, write-synchronized. Write 0xA5 to pet the watchdog. Any other value causes
an immediate system reset. Wait SYNCBUSY.CLEAR=0 before writing again.

## SYNCBUSY Bits

| Bit | Name | Wait after |
|-----|------|-----------|
| 4 | CLEAR | CLEAR write |
| 3 | ALWAYSON | CTRLA.ALWAYSON write |
| 2 | WEN | CTRLA.WEN write |
| 1 | ENABLE | CTRLA.ENABLE write |

## Early Warning Interrupt

EWCTRL.EWOFFSET[3:0] sets the early warning period offset (same encoding as PER).
The EW interrupt fires before the actual timeout, allowing the application to decide
whether to clear or let the reset happen. EWOFFSET must be less than PER.

## Key Facts

- CLK_WDT_OSC is derived from OSCULP32K (~1 kHz) — not precise, do not rely on exact ms values.
- CTRLA, CONFIG, EWCTRL reset values are loaded from NVM User Row bits — factory-configured default.
- ALWAYSON cannot be cleared by software after it is set; requires POR to disable.
- Window mode: clearing too early (CLEAR write while window is closed) triggers a reset.
- In normal mode, CLEAR can be written at any time during the timeout period.
- CONFIG must be written while the WDT is disabled (ENABLE=0), except for ALWAYS-ON mode where
  it can be written any time before the WDT times out.
- SYNCBUSY must be checked before writing CLEAR (cannot write twice without waiting).

## See Also

- [[WDT]] — firmware usage and init pattern
- [[Clock System]] — OSCULP32K clock source for WDT
- [[Memory Map]] — WDT base address 0x40002000
- [[NVIC Interrupt Map]] — WDT IRQn
