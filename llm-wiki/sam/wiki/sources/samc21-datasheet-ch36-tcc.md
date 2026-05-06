---
title: SAMC21 Datasheet Ch.36 TCC
type: source
tags: [tcc, timer, pwm, capture, samc21, datasheet]
sources: [samc21-datasheet-ch36-tcc]
created: 2026-05-05
updated: 2026-05-05
---

# SAMC21 Datasheet Ch.36 TCC

Timer/Counter for Control Applications. Three instances (TCC0, TCC1, TCC2) with
advanced PWM generation, dead-time insertion, pattern generation, and recoverable
fault handling. Distinct from TC: TCC is oriented toward motor control and
power converters.

## Abstract

The TCC module provides up to 24-bit counting, up to 4 compare/capture channels
per instance, single- and dual-slope PWM, output matrix with dead-time insertion,
BLDC pattern generation, and two independent recoverable fault inputs (FaultA/B)
with filtering, blanking, qualification, keep, restart, and halt actions.
Non-recoverable faults force outputs to a safe state immediately. DMA, event, and
interrupt integration covers overflow, compare match, capture, and fault conditions.

## Key Facts

- Three instances: TCC0, TCC1, TCC2. TCC0 and TCC1 share the same GCLK_TCC peripheral
  channel; TCC2 has its own.
- Counter width: TCC0 24-bit, TCC1 24-bit, TCC2 16-bit (instance-dependent).
- Up to 4 CC channels (CC0–CC3); not all instances support all 4.
- Up to 8 WO (waveform output) pins per instance through output matrix (OTMX).
- Enable-protected registers: CTRLA (excl. RUNSTDBY/ENABLE/SWRST), FCTRLA, FCTRLB,
  WEXCTRL, DRVCTRL, EVCTRL.
- SYNCBUSY must be polled after writes to CTRLB, STATUS, PATT, PATTBUF, WAVE,
  WAVEBUF, COUNT, PER, PERBUF, CCx, CCBUFx.
- COUNT read requires READSYNC: write CTRLBSET.CMD=READSYNC, poll SYNCBUSY.COUNT.
- Double buffering: CCBUFx→CCx and PERBUF→PER at the UPDATE condition.
  CTRLB.LUPD=1 locks updates (prevents buffer copy). CTRLA.ALOCK auto-sets LUPD
  on each overflow/underflow/re-trigger.
- Dithering: CTRLA.RESOLUTION = DITH4/5/6 → spread extra cycles over 16/32/64 frames.
  Lower bits of PER/CCx become the dither pattern.
- Master/slave synchronization: CTRLA.MSYNC=1 on slave links its CC channels to the
  master TCC counter (both must share the same GCLK_TCC).
- RUNSTDBY=1: TCC continues running in standby.

## Prescaler

CTRLA.PRESCALER[2:0]:

| Value | Name | Factor |
|-------|------|--------|
| 0x0 | DIV1 | 1 |
| 0x1 | DIV2 | 2 |
| 0x2 | DIV4 | 4 |
| 0x3 | DIV8 | 8 |
| 0x4 | DIV16 | 16 |
| 0x5 | DIV64 | 64 |
| 0x6 | DIV256 | 256 |
| 0x7 | DIV1024 | 1024 |

## Waveform Modes (WAVE.WAVEGEN)

| Value | Name | Description |
|-------|------|-------------|
| 0x0 | NFRQ | Normal frequency: toggle on compare match, TOP=PER |
| 0x1 | MFRQ | Match frequency: toggle on compare match, TOP=CC0 |
| 0x2 | NPWM | Normal single-slope PWM: set on ZERO, clear on CCx |
| 0x4 | DSTOP | Dual-slope: update at TOP, PWM set/clear at CCx |
| 0x5 | DSCRITICAL | Dual-slope: update at ZERO and TOP (critical section) |
| 0x6 | DSBOTTOM | Dual-slope: update at ZERO |
| 0x7 | DSBOTH | Dual-slope: update at ZERO and TOP |

### PWM Frequency Formulas

Single-slope (NPWM): `f_PWM = f_GCLK / (N × (PER + 1))`  
Dual-slope (DSxxx): `f_PWM = f_GCLK / (2 × N × PER)`  
where N = prescaler factor.

## Output Matrix (WEXCTRL.OTMX)

Distributes compare channels to WO[0:7] output pins:

| OTMX | WO[3] | WO[2] | WO[1] | WO[0] | WO[7] | WO[6] | WO[5] | WO[4] |
|------|-------|-------|-------|-------|-------|-------|-------|-------|
| 0x0 | CC3 | CC2 | CC1 | CC0 | CC3 | CC2 | CC1 | CC0 |
| 0x1 | CC1 | CC0 | CC1 | CC0 | CC1 | CC0 | CC1 | CC0 |
| 0x2 | CC0 | CC0 | CC0 | CC0 | CC0 | CC0 | CC0 | CC0 |
| 0x3 | CC1 | CC1 | CC1 | CC0 | CC1 | CC1 | CC1 | CC0 |

0x0: default 1:1 channel-to-output. 0x1: pairs for H-bridge. 0x2: single CC0
broadcast (stepper motor). 0x3: 7× LED strings.

## Dead-Time Insertion (WEXCTRL)

- DTIEN0–DTIEN3: enable DTI for each channel pair.
- WEXCTRL.DTLS[7:0]: low-side dead time (rising edge of OTMX output).
- WEXCTRL.DTHS[7:0]: high-side dead time (falling edge of OTMX output).
- 8-bit counter decremented each prescaled clock cycle.
- Dead time = DTLS (or DTHS) / f_GCLK_TCC_PRESC.
- Both LS and HS outputs forced OFF while counter > 0.

## Pattern Generation (PATT)

- PATT.PGE[7:0]: enable pattern override on WO[x].
- PATT.PGV[7:0]: value driven on WO[x] when PGE[x]=1.
- PATTBUF (0x64) is the double-buffered shadow; copied to PATT at UPDATE.
- Used for BLDC commutation sequences and stepper motor control.

## Ramp Modes (WAVE.RAMP)

- RAMP1: standard counting (default).
- RAMP2: alternating cycle A (compare against CC0) and cycle B (compare against CC1),
  directed by STATUS.IDX.
- RAMP2A: RAMP2 but direction changes automatically at each TOP.
- RAMP2C: RAMP2 with automatic cycle selection based on counter direction.

## Recoverable Faults (FCTRLA / FCTRLB)

Configure FaultA (via event channel EV0) and FaultB (via EV1) independently.

Key FCTRLn fields:
- `SRC[1:0]`: fault source (disabled / event / inverted event / pin)
- `KEEP`: clamp output to zero while fault active
- `QUAL`: fault input disabled when output is inactive (prevents false triggers)
- `BLANK[1:0]`: edge-triggered blank window; `BLANKVAL[7:0]`: duration in prescaled clocks
- `FILTERVAL[3:0]`: digital filter (must be < event width to be valid)
- `HALT[1:0]`: 0=none, 1=hardware halt (auto-resume when fault clears), 2=software halt
- `RESTART`: restart counter at start of new cycle after fault
- `CHSEL[1:0]`: which compare channel generates fault input event
- `CAPTURE[2:0]`: capture action (CAPT / CAPTMIN / CAPTMAX / LOCMIN / LOCMAX / DERIV0)

Fault blanking time: `t_b = (1 + BLANKVAL) / f_GCLK_TCC_PRESC`

## Non-Recoverable Faults (DRVCTRL)

- NRE[7:0]: non-recoverable event enable on WO[x].
- NRV[7:0]: forced output value on WO[x] when non-recoverable fault active.
- INVEN[7:0]: output inversion.
- FILTERVAL0/1[3:0]: digital filter for EV0/EV1 non-recoverable fault inputs.
- Activated by EVCTRL.EVACT0/1 = FAULT.

## Capture Operations (via CPTENx + MCEIx event)

- CAPT: standard timestamped capture on event.
- CAPTMIN: capture minimum value; MCx interrupt on new local minimum.
- CAPTMAX: capture maximum value; MCx interrupt on new local maximum.
- LOCMIN/LOCMAX: notify event/interrupt when detected (CCx follows counter).
- DERIV0: OR of LOCMIN and LOCMAX.
- PPW: period→CC0, pulse width→CC1 (enable on EV1 with EVACT1=PPW).

## DMA

- OVF DMA request: on overflow/underflow/re-trigger (if CTRLA.DMAOS=0) or
  one-shot mode (CTRLA.DMAOS=1 + CTRLBSET.CMD=DMAOS).
- MC DMA request: on compare match (DMAOS=0) or capture data available.
- Circular buffer DMA: RAMP2/RAMP2A/DSBOTH — two DMA channels interleave
  buffer updates for ramp A (triggered by MCx) and ramp B (triggered by OVF).

## Interrupts

INTFLAG / INTENSET / INTENCLR bits:

| Bit | Name | Source |
|-----|------|--------|
| OVF | Overflow/Underflow | counter overflow or underflow |
| TRG | Retrigger | re-trigger event |
| CNT | Counter | EVCTRL.CNTSEL event |
| ERR | Error | capture overflow |
| UFS | Non-Recoverable Update Fault | LUPD=1 with update in RAMP/DSBOTH |
| DFS | Debug Fault | debug break with DBGCTRL.FDDBD=1 |
| FAULTA | Recoverable Fault A | |
| FAULTB | Recoverable Fault B | |
| FAULT0 | Recoverable Fault 0 (ch0) | |
| FAULT1 | Recoverable Fault 1 (ch1) | |
| MC0–MC3 | Compare Match/Capture ch.0–3 | |

## Synchronization

Write-synchronized (poll SYNCBUSY after write):
CTRLB, STATUS, PATT, PATTBUF, WAVE, WAVEBUF, COUNT, PER, PERBUF, CCx, CCBUFx.

Read-synchronized (read-back is stale until sync):
CTRLB, PATT, PATTBUF, WAVE, WAVEBUF, PER, PERBUF, CCx, CCBUFx.
COUNT: demand-sync via CTRLBSET.CMD=READSYNC → poll SYNCBUSY.COUNT.

CTRLA.ENABLE/SWRST write-synchronized; poll SYNCBUSY.ENABLE/SWRST.

## Register Summary

| Offset | Name | Key Fields |
|--------|------|-----------|
| 0x00 | CTRLA | SWRST, ENABLE, RESOLUTION[1:0], PRESCALER[2:0], RUNSTDBY, PRESCYNC[1:0], ALOCK, MSYNC, CPTEN0–3 |
| 0x04 | CTRLBCLR | DIR, LUPD, ONESHOT, IDXCMD[1:0], CMD[2:0] |
| 0x05 | CTRLBSET | (same bits — set by writing 1) |
| 0x08 | SYNCBUSY | SWRST, ENABLE, CTRLB, STATUS, PATT, WAVE, COUNT, PER, CC0–3 |
| 0x0C | FCTRLA | SRC, KEEP, QUAL, BLANK, HALT, CHSEL, CAPTURE, BLANKVAL, FILTERVAL, RESTART |
| 0x10 | FCTRLB | (same structure as FCTRLA, for Fault B) |
| 0x14 | WEXCTRL | OTMX[1:0], DTIEN0–3, DTLS[7:0], DTHS[7:0] |
| 0x18 | DRVCTRL | NRE0–7, NRV0–7, INVEN0–7, FILTERVAL0/1[3:0] |
| 0x1E | DBGCTRL | DBGRUN, FDDBD |
| 0x20 | EVCTRL | OVFEO, TRGEO, CNTEO, TCINV0/1, TCEI0/1, EVACT0[2:0], EVACT1[2:0], MCEI0–3, MCEO0–3, CNTSEL[1:0] |
| 0x24 | INTENCLR | OVF, TRG, CNT, ERR, UFS, DFS, FAULTA, FAULTB, FAULT0, FAULT1, MC0–3 |
| 0x28 | INTENSET | (same bits) |
| 0x2C | INTFLAG | (same bits — write 1 to clear) |
| 0x30 | STATUS | STOP, IDX, UFS, DFS, SLAVE, PATTBUFV, WAVEBUFV, PERBUFV, FAULTAIN, FAULTBIN, FAULT0IN, FAULT1IN, FAULTA, FAULTB, FAULT0, FAULT1, CCBUFV0–3, CMP0–3 |
| 0x34 | COUNT | COUNT[23:0] (read-sync via READSYNC) |
| 0x38 | PATT | PGE[7:0], PGV[7:0] (write-sync) |
| 0x3C | WAVE | WAVEGEN[2:0], CIPEREN, CICCEN0–3, POL0–3, SWAP0–3 (write-sync) |
| 0x40 | PER | DITHER[5:0] (when DITH4/5/6), PER[17:0] (write-sync) |
| 0x44–0x50 | CC0–CC3 | DITHER[5:0], CC[17:0] (write-sync) |
| 0x64 | PATTBUF | PGEB[7:0], PGVB[7:0] (double-buffer for PATT) |
| 0x68 | WAVEBUF | WAVEGENB, CIPERENB, CICCENB0–3, POLB0–3, SWAPB0–3, RAMPB[1:0] |
| 0x6C | PERBUF | DITHERBUF, PERBUF[17:0] |
| 0x70–0x7C | CCBUF0–3 | DITHERBUF, CCBUF[17:0] |

## See Also

- [[TC 32-Bit Paired Mode]] — simpler TC timer (no PWM extensions)
- [[Clock System]] — GCLK_TCC setup
- [[NVIC Interrupt Map]] — TCC0/1/2 IRQn values
- [[I/O Multiplexing]] — TCC WO pin assignments
- [[EVSYS]] — event routing for fault/capture/count inputs
