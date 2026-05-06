---
title: SAMC21 Datasheet Chapter 19 — PM
type: source
tags: [pm, power, sleep, standby, samc21, datasheet]
date: 2026-05-05
---

# SAMC21 Datasheet Ch. 19 — PM – Power Manager

Source: SAM C20/C21 Data Sheet DS60001479D, chapter 19, pages 181–189.

## Overview

The Power Manager (PM) controls the MCU sleep modes. It provides two sleep
modes: IDLE (CPU halted, peripherals continue) and STANDBY (deep sleep,
GCLK stopped, RAM back-biased). Sleep is entered by writing SLEEPCFG then
executing the WFI instruction.

## Sleep Modes (SLEEPCFG.SLEEPMODE)

| Value | Name | Description |
|-------|------|-------------|
| 0x0 | Reserved | — |
| 0x1 | Reserved | — |
| 0x2 | IDLE | CPU stopped; AHB/APB clocks gated on demand; GCLK0/MCLK run; RAM normal; main regulator |
| 0x3 | Reserved | — |
| 0x4 | STANDBY | CPU stopped; AHB/APB stopped; GCLK stopped (unless RUNSTDBY=1 on generator); RAM back-biased; LP regulator |

Values 0x5–0x7 are reserved; writing them has undefined behavior.

## Entering Sleep

```cpp
PM->SLEEPCFG.reg = PM_SLEEPCFG_SLEEPMODE_STANDBY;
while (PM->SLEEPCFG.reg != PM_SLEEPCFG_SLEEPMODE_STANDBY);  // read-back verify
__WFI();
```

Read-back after write is mandatory: the register is write-synchronized and the
CPU must not execute WFI until the new mode is active in the PM state machine.

## IDLE Mode

- CPU core and SysTick stopped.
- AHB and APB clock gating: active only for peripherals that have pending requests.
- MCLK and GCLK generators continue running.
- RAM powered at full voltage (no back-bias).
- Main voltage regulator active.
- Wake: any interrupt (NVIC-enabled) or NMI.

## STANDBY Mode

- CPU core stopped.
- AHB/APB clocks stopped.
- All GCLK generators stopped, unless their RUNSTDBY bit is set.
- Peripherals with RUNSTDBY=1 in their CTRLA remain active (e.g., RTC, WDT).
- RAM powered in back-bias mode (BBIASHS=1 in STDBYCFG → back-bias high side).
- LP voltage regulator active (set via SUPC VREG or STDBYCFG.VREGSMOD).
- Wake: asynchronous interrupt (RTC alarm, EIC edge, WDT early warning, etc.).

## STDBYCFG Register (0x08)

| Bits | Name | Description |
|------|------|-------------|
| 10 | BBIASHS | RAM back-bias high side; reset=1 (back-bias enabled) |
| 7:6 | VREGSMOD[1:0] | Voltage regulator mode in standby: AUTO(0) / PERFORMANCE(1) / LP(2) |

AUTO mode lets the regulator switch automatically based on active loads.
LP mode maximizes current savings when wake latency is acceptable.

## Register Map

| Offset | Name | Reset | Description |
|--------|------|-------|-------------|
| 0x01 | SLEEPCFG | 0x02 | SLEEPMODE[2:0] — sleep mode select |
| 0x08 | STDBYCFG | 0x0400 | BBIASHS(10)/VREGSMOD[7:6] — standby config |

## Key Facts

- SLEEPMODE 0x0/0x1/0x3/0x5-0x7 are reserved; only 0x2 (IDLE) and 0x4 (STANDBY) are valid.
- Must read SLEEPCFG back and verify match before executing WFI.
- In STANDBY, peripherals with RUNSTDBY=1 (WDT, RTC, EIC) remain clocked.
- BBIASHS reset=1: back-bias is on by default; this slightly degrades RAM
  retention/performance but reduces standby current.
- VREGSMOD default AUTO (0x0): suitable for most applications.
- Wake from STANDBY restores full clock tree; wait for PLLs/oscillators to lock
  if they were stopped.

## See Also

- [[PM]] — firmware sleep entry patterns
- [[SUPC]] — voltage regulator and BOD configuration
- [[Clock System]] — GCLK RUNSTDBY, PCHCTRL
- [[WDT]] — always-on peripheral, runs in standby
