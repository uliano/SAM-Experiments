---
title: SAMC21 Datasheet Ch.15 Clock System
type: source
tags: [clock, gclk, mclk, oscctrl, syncbusy, samc21, datasheet]
sources: [samc21-datasheet-ch15-clock-system]
created: 2026-05-05
updated: 2026-05-05
---

# SAMC21 Datasheet Ch.15 Clock System

Overview of clock distribution, synchronization mechanisms, on-demand clocking,
and reset behavior for the SAM C20/C21.

## Abstract

Chapter 15 is a conceptual summary of the clock system — not a register reference
(see Ch.16 GCLK and Ch.17 MCLK for registers). It explains the three-tier clock
hierarchy (clock sources → GCLK generators → peripheral channels), the
synchronous vs. asynchronous clock distinction, SYNCBUSY semantics, and the
on-demand power-saving mechanism. Reset state and write-lock behavior are also
documented here.

## Key Facts

- Three clock sources: OSCCTRL (OSC48M, XOSC, FDPLL96M) and OSC32KCTRL
  (XOSC32K, OSC32K, OSCULP32K).
- GCLK generators are programmable prescalers from any clock source.
- GCLK_MAIN = GCLK Generator 0; feeds MCLK synchronous clock controller.
- MCLK controls CPU clock, AHB bus clocks, and APB peripheral bus clocks.
- Each peripheral has two clock domains:
  - APB/AHB bus clock (synchronous, from MCLK) — controls register access.
  - Generic clock (asynchronous, from GCLK) — drives peripheral core logic.
- SYNCBUSY: must be polled before writing to a "write-synchronized" core register.
- A second write to the same core register while synchronization is ongoing is
  **discarded** and generates a PAC error.
- Synchronization delay: `5·PGCLK + 2·PAPB < D < 6·PGCLK + 3·PAPB`
  where PGCLK = generic clock period, PAPB = peripheral APB clock period.
- CTRLA.ENABLE and CTRLA.SWRST are always write-synchronized.
- On-demand (ONDEMAND bit): clock source stops when no peripheral is requesting it.
- RUNSTDBY bit: keeps the clock source running in STANDBY sleep mode.
- WRTLOCK: a generic clock with WRTLOCK=1 survives a user reset (only cleared by power-on reset).

## Reset State

On **any reset** (power-on or user reset), synchronous clocks revert to initial state:
- OSC48M enabled and divided by 12 → effective GCLK_MAIN = 4 MHz.
- GCLK_MAIN (generator 0) uses OSC48M source.
- CPU and BUS clocks undivided.

On **power-on reset**, GCLK starts in initial state:
- Only GCLK generator 0 (GCLK_MAIN) active, source = OSC48M, no division.
- All other GCLK generators: disabled.
- All generic clocks (PCHCTRL): disabled.

On **user reset (SWRST)**, GCLK resets as in power-on except:
- Generic clocks with WRTLOCK=1 survive (keep their generator assignment).

On any reset, **32kHz clock sources** (XOSC32K, OSC32K, OSCULP32K) are only reset
by power-on reset, not by user reset.

## Enabling a Peripheral (Full Sequence)

1. Ensure clock source is running (e.g., OSC48M is running by default at reset).
2. Configure GCLK generator (GCLK→GENCTRL[n]) with source + divider + GENEN.
3. Connect peripheral channel (GCLK→PCHCTRL[m]) with generator index + CHEN.
4. Enable APB/AHB clock mask in MCLK (e.g., MCLK→APBCMASK |= ...).
5. Configure and enable the peripheral (set CTRLA.ENABLE, poll SYNCBUSY).

Steps 2 and 3 are required only for peripherals that use a generic clock.
APB-A peripherals are clocked at reset — steps 2–4 may be skipped.

## On-Demand Clock Requests

Clock requests propagate from the peripheral (PCHCTRL.CLKEN=1), through the
GCLK generator (GENCTRL.GENEN=1), to the clock source (ENABLE=1 + ONDEMAND).
When no peripheral is actively requesting, the chain can power down automatically.

ONDEMAND can be disabled per clock source to keep it always running — at the cost
of static power consumption. Disable ONDEMAND when the clock source startup
latency would violate timing requirements.

## Synchronization Rules

- Read-synchronized registers: check SYNCBUSY before reading (rare — only a few registers in some peripherals).
- Write-synchronized registers: check SYNCBUSY before writing again.
- Bus interface registers (not core registers): no synchronization needed.
- Multiple write-synchronized registers in the same peripheral can be written
  concurrently (synchronization is per-register, not per-peripheral), EXCEPT:
  writing a 32-bit value that spans two synchronized registers applies independently.

## Concepts Mentioned

- [[Clock System]] — concept page with practical init sequences
- [[OSCCTRL]] — OSC48M, XOSC, FDPLL96M configuration
- [[OSC32KCTRL]] — XOSC32K, OSC32K, OSCULP32K configuration
- [[GCLK]] (via [[Clock System]]) — generator configuration
- [[MCLK]] (via [[Clock System]]) — APB mask configuration

## See Also

- [[Clock System]] — GCLK/MCLK practical init and PCHCTRL usage
- [[OSCCTRL]] — clock source configuration details
- [[OSC32KCTRL]] — 32kHz clock sources
- [[SAMC21 Datasheet Ch.16 GCLK]] — GCLK register reference
- [[SAMC21 Datasheet Ch.17 MCLK]] — MCLK register reference
- [[SAMC21 Datasheet Ch.12 Peripherals Configuration Summary]] — GCLK PCHCTRL index table
