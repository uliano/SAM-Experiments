---
title: SAMC21 Datasheet Ch.40 AC
type: source
tags: [ac, comparator, analog, samc21, datasheet]
sources: [samc21-datasheet-ch40-ac]
created: 2026-05-05
updated: 2026-05-05
---

# SAMC21 Datasheet Ch.40 AC

Analog Comparators. Four individual comparators grouped in two pairs:
(COMP0, COMP1) and (COMP2, COMP3). Requires GCLK_AC for filtering and
synchronized output, but continuous mode can detect asynchronously (clockless
in STANDBY).

## Abstract

Each comparator has configurable positive/negative input mux, programmable speed,
hysteresis, digital filter, output pin routing, and window mode. Single-shot or
continuous operation. Events and interrupts for each comparator and each window
pair. VDD scaler with 64 levels available as a reference input.

## Key Facts

- Four comparators: COMP0/COMP1 (pair 0), COMP2/COMP3 (pair 1).
- Inputs: AIN[7:0] (positive and negative mux), internal DAC, VDD scaler,
  bandgap, ground.
- Outputs: CMP[3:0] digital output pins, optional event outputs.
- GCLK_AC required for filtering and output synchronization.
- Only EVCTRL is enable-protected; all other registers except CTRLA are NOT
  enable-protected.
- Write-synchronized (poll SYNCBUSY): CTRLA.SWRST, CTRLA.ENABLE,
  COMPCTRLn.ENABLE, WINCTRL.
- COMPCTRLn is the main per-comparator 32-bit control register.
- MUXPOS[2:0] selects positive input; MUXNEG[2:0] selects negative input.
- SPEED[1:0]: power/speed tradeoff (0=lowest power, 3=fastest).
- HYSTEN: hysteresis enable. Available in continuous mode only.
- FLEN[2:0]: digital filter length — 0=none, 1=3-bit majority, 2=5-bit majority;
  each step adds N−1 delay cycles at GCLK_AC rate.
- OUT[1:0]: comparator output routing — 0=off, 1=async raw, 2=sync to GCLK_AC.
- SWAP: swap MUXPOS and MUXNEG (offset compensation measurement); only valid
  while comparator is disabled.
- SINGLE: 0=continuous, 1=single-shot. Single-shot starts via CTRLB.STARTx or
  COMPEI event.
- INTSEL[1:0]: interrupt/event generation — toggle(0), rising(1), falling(2),
  end-of-comparison(3, single-shot only).
- RUNSTDBY: comparator runs in STANDBY if set; continuous mode detects
  asynchronously without GCLK_AC.

## Operating Modes

**Continuous mode (SINGLE=0):**
- Comparator always active; output available after startup delay.
- STATUSB.READYx goes high when output is valid.
- Hysteresis and filtering available.

**Single-shot mode (SINGLE=1):**
- Triggered by CTRLB.STARTx write or COMPEIx event.
- Comparator powers on, compares, updates STATUSA, fires INTFLAG, powers off.
- No hysteresis, no filtering.
- INTSEL=0x3 (end-of-comparison) is the typical interrupt mode.

## Window Mode

- Enabled per pair: WINCTRL.WEN0 (COMP0+COMP1), WINCTRL.WEN1 (COMP2+COMP3).
- Both comparators in the pair must share the same positive input (same AIN pin).
- COMP1/COMP3 uses the low threshold; COMP0/COMP2 uses the high threshold.
- STATUSA.WSTATE0/1: ABOVE(0x0), INSIDE(0x1), BELOW(0x2).
- Both comparators in the pair must have the same SINGLE setting.

## VDD Scaler

- 64-level resistor ladder: output = V_DD × (VALUE[5:0]+1) / 64.
- Used when MUXNEG=0x5 or MUXPOS=0x4.
- SCALERn.VALUE[5:0] is per-comparator (SCALERn offset: 0x1C for COMP0, 0x1D for COMP1, etc.).

## Interrupts

- Per-comparator: COMP0, COMP1, COMP2, COMP3.
- Per window pair: WIN0, WIN1.
- INTSEL controls trigger condition.
- INTFLAG bits: write 1 to clear.

## Events

- Outputs: COMPEOx (comparator state change per INTSEL condition), WINEOx (window state change).
- Inputs: COMPEIx (start comparison in single-shot mode).
- INVEI0-3: invert COMPEIx input event signal.
- EVCTRL (enable-protected, offset 0x02, 16-bit):
  COMPEO0-3[3:0], WINEO0-1[5:4], COMPEI0-3[8:11], INVEI0-3[12:15].

## Status Registers

- STATUSA (read-only): STATE0-3 (comparator outputs, valid when READY=1),
  WSTATE0 (pair 0 window: ABOVE/INSIDE/BELOW), WSTATE1 (pair 1 window).
- STATUSB (read-only): READY0-3 (comparator output valid flag).

## Sleep / Low-Power

- RUNSTDBY=1 per COMPCTRLn: comparator active in STANDBY.
- Continuous mode: asynchronous detection does not require GCLK_AC.
- Can be wakeup source from STANDBY.

## Synchronization

Write-synchronized (poll SYNCBUSY):
- SWRST, ENABLE (via SYNCBUSY.SWRST/ENABLE).
- COMPCTRL0, COMPCTRL1, COMPCTRL2, COMPCTRL3, WINCTRL (separate SYNCBUSY bits).

## Register Summary

| Offset | Name | Key Fields |
|--------|------|-----------|
| 0x00 | CTRLA | SWRST, ENABLE, RUNSTDBY (write-sync: SWRST, ENABLE) |
| 0x01 | CTRLB | START0, START1, START2, START3 (single-shot triggers) |
| 0x02 | EVCTRL | COMPEO0-3, WINEO0-1, COMPEI0-3, INVEI0-3 (16-bit, enable-protected) |
| 0x04 | INTENCLR | WIN1, WIN0, COMP3, COMP2, COMP1, COMP0 |
| 0x05 | INTENSET | (same) |
| 0x06 | INTFLAG | (same — write 1 to clear) |
| 0x07 | STATUSA | WSTATE1[1:0], WSTATE0[1:0], STATE3, STATE2, STATE1, STATE0 (read-only) |
| 0x08 | STATUSB | READY3, READY2, READY1, READY0 (read-only) |
| 0x0C | DBGCTRL | DBGRUN |
| 0x10 | COMPCTRL0 | RUNSTDBY, ENABLE, SINGLE, INTSEL[1:0], MUXNEG[2:0], MUXPOS[2:0], SWAP, SPEED[1:0], HYSTEN, FLEN[2:0], OUT[1:0] (32-bit, write-sync) |
| 0x14 | COMPCTRL1 | (same as COMPCTRL0) |
| 0x18 | COMPCTRL2 | (same as COMPCTRL0) |
| 0x1C(?) | COMPCTRL3 | (same as COMPCTRL0) |
| +0x00 | SCALER0 | VALUE[5:0] (VDD scaler for COMP0) |
| +0x01 | SCALER1 | VALUE[5:0] (VDD scaler for COMP1) |
| +0x02 | SCALER2 | VALUE[5:0] (VDD scaler for COMP2) |
| +0x03 | SCALER3 | VALUE[5:0] (VDD scaler for COMP3) |
| 0x20 | SYNCBUSY | SWRST, ENABLE, COMPCTRL0, COMPCTRL1, COMPCTRL2, COMPCTRL3, WINCTRL (read-only) |

## See Also

- [[SAMC21 Datasheet Ch.26 EIC]] — external interrupt controller (digital, edge/level)
- [[EVSYS]] — event routing for AC comparator triggers
- [[SUPC]] — VREF and supply voltage for analog measurements
- [[Clock System]] — GCLK_AC setup
