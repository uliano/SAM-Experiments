---
title: SAMC21 Datasheet Chapter 18 — RSTC
type: source
tags: [rstc, reset, samc21, datasheet]
date: 2026-05-05
---

# SAMC21 Datasheet Ch. 18 — RSTC – Reset Controller

Source: SAM C20/C21 Data Sheet DS60001479D, chapter 18, pages 176–180.

## Overview

The Reset Controller (RSTC) records the cause of the last reset in the RCAUSE
register. It has no interrupts, no DMA triggers, and no event outputs. Reading
RCAUSE allows firmware to distinguish between power-on, brown-out, watchdog,
external, and software resets, and take appropriate recovery action.

## Base Address

RSTC is an always-on peripheral on the APBA bus. Base address is within the
standard APBA range (refer to [[Memory Map]] for exact address).

## Reset Sources

| RCAUSE Bit | Name | Trigger |
|-----------|------|---------|
| 0 | POR | Power-On Reset (VDD ramp) |
| 1 | BODCORE | Brown-out detected on VDDCORE |
| 2 | BODVDD | Brown-out detected on VDD (from SUPC BODVDD) |
| 4 | EXT | External reset via RESET# pin |
| 5 | WDT | Watchdog Timer timeout or early window violation |
| 6 | SYST | System reset request (CPU SYSRESETREQ / NVIC SystemReset()) |

Only one bit is set at a time (the most recent reset cause).

## Reset Scope by Type

Resets are divided into two groups with different scope:

### Power Supply Resets (POR / BODCORE / BODVDD)

Reset the following subsystems only:
- RTC
- OSC32KCTRL
- RSTC itself
- GCLK registers with WRTLOCK set

**Do NOT** reset other peripherals. Firmware state and most peripheral
configuration may survive a supply-triggered reset.

### User Resets (EXT / WDT / SYST)

Reset everything — all peripherals, CPU, SRAM contents undefined after reset.
These are equivalent to a full chip reset from the application perspective.

## Register Map

| Offset | Name | Reset | Description |
|--------|------|-------|-------------|
| 0x00 | RCAUSE | (power-on) | Reset cause flags (read-only) |

RCAUSE is read-only. Bits are cleared only on the next reset event.

## Key Facts

- RCAUSE is the only register; no configuration, no interrupts.
- POR/BOD resets do NOT reset all peripherals — check RCAUSE on startup if
  peripheral state must be validated.
- Firmware should read RCAUSE early in startup (before sys_init may overwrite
  clocks) and preserve the value for diagnostics.
- SYST reset (via `SCB->AIRCR = SCB_AIRCR_SYSRESETREQ_Msk | key`) triggers
  a full user-class reset.

## See Also

- [[RSTC]] — firmware reset-cause reading pattern
- [[WDT]] — watchdog reset source
- [[SUPC]] — BODVDD configuration
- [[Memory Map]] — RSTC base address
