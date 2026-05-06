---
title: SAMC21 Datasheet Ch.43 TSENS
type: source
tags: [tsens, temperature, analog, samc21, datasheet]
sources: [samc21-datasheet-ch43-tsens]
created: 2026-05-05
updated: 2026-05-05
---

# SAMC21 Datasheet Ch.43 TSENS

Temperature Sensor. Measures the operating temperature of the device using a
time-domain technique. Present on both SAM C20 and SAM C21.

## Abstract

The TSENS measures temperature by comparing the difference of two
temperature-dependent oscillator (TOSC) frequencies against a known GCLK_TSENS
frequency. The result VALUE is a 24-bit signed integer in two's complement. With
NVM calibration values loaded, resolution is 100 counts per °C (25°C → VALUE =
2500). GAIN, OFFSET, FCAL, and TCAL must be loaded from the NVM Temperature
Calibration Area before use.

## Key Facts

- Present on both SAM C20 and SAM C21. One instance.
- Result: 24-bit signed (two's complement) in VALUE register.
- Resolution: 100 counts = 1°C when NVM calibration loaded and GCLK_TSENS=48MHz (OSC48M).
  Example: T=25°C → VALUE=2500=0x09C4; T=−25°C → VALUE=−2500=0xFFF63C.
- Measurement formula: `VALUE = OFFSET + GAIN × (f_TOSCMIN/f_GCLK − f_TOSCMAX/f_GCLK)`
- GAIN factory-calibrated for OSC48M as GCLK_TSENS. Other frequencies require
  GAIN to be scaled accordingly.
- **Calibration mandatory**: GAIN, OFFSET, FCAL, TCAL must be read from NVM
  Temperature Calibration Area (section 9.4 / NVMCTRL_TEMP_LOG) before enabling.
- GAIN and OFFSET are NOT reset by CTRLA.SWRST — they persist across software resets.
- Enable-protected: CTRLA.RUNSTDBY, CTRLC, EVCTRL, WINLT, WINUT, GAIN, OFFSET, CAL.
- Write-synchronized (poll SYNCBUSY): CTRLA.SWRST, CTRLA.ENABLE.
  Bus error if written while SYNCBUSY set.
- No read-synchronized registers.
- Average of 10 measurements recommended by datasheet for accuracy.
- Single measurement: CTRLB.START=1 (write-only, no SYNCBUSY).
- Free-running: CTRLC.FREERUN=1 (auto-restart after each measurement).
- STATUS.OVF=1: result required >24 bits — VALUE is invalid.

## Principle of Operation

Two TOSC configurations (min/max frequency) are measured over a known number of
GCLK_TSENS periods defined by GAIN. The counter measures in up-direction during
phase 1 (min config) and down-direction during phase 2 (max config). The net
count, corrected by OFFSET, is the signed VALUE proportional to temperature.

## Enable-Protected Registers

Configure all of these before CTRLA.ENABLE=1:
- CTRLA.RUNSTDBY
- CTRLC (FREERUN, WINMODE)
- EVCTRL (WINEO, STARTINV, STARTEI)
- WINLT (24-bit signed threshold)
- WINUT (24-bit signed threshold)
- GAIN (24-bit, from NVM)
- OFFSET (24-bit signed, from NVM)
- CAL (FCAL[5:0]+TCAL[5:0], from NVM)

## Window Monitor Modes (CTRLC.WINMODE)

| Value | Name | Condition |
|-------|------|-----------|
| 0x0 | DISABLE | No window monitor |
| 0x1 | ABOVE | VALUE > WINLT |
| 0x2 | BELOW | VALUE < WINUT |
| 0x3 | INSIDE | WINLT < VALUE < WINUT |
| 0x4 | OUTSIDE | WINUT < VALUE < WINLT |
| 0x5 | HYST_ABOVE | VALUE > WINUT with hysteresis to WINLT |
| 0x6 | HYST_BELOW | VALUE < WINLT with hysteresis to WINUT |

## Sleep Behavior

| RUNSTDBY | FREERUN | ENABLE | Behavior |
|----------|---------|--------|----------|
| 0 | 0 | 1 | Runs in all sleep modes on request, except STANDBY |
| 0 | 1 | 1 | Runs in all sleep modes, except STANDBY |
| 1 | 0 | 1 | Runs in all sleep modes on request |
| 1 | 1 | 1 | Runs in all sleep modes |

## DMA

DMA trigger: RESRDY. Request cleared when VALUE register is read.

## Interrupts

- RESRDY: result ready; cleared by writing 1 or reading VALUE.
- WINMON: window monitor condition matched; cleared by writing 1 or reading VALUE.
- OVERRUN: new result ready before previous was read.
- OVF: result overflowed (>24 bits required); VALUE is invalid.

## Events

- Output: WINEO — window monitor condition matched (EVCTRL.WINEO=1).
- Input: STARTEI — starts a measurement (EVCTRL.STARTEI=1).
- STARTINV: invert STARTEI edge (0=rising, 1=falling).

## Register Summary

| Offset | Name | Key Fields |
|--------|------|-----------|
| 0x00 | CTRLA | RUNSTDBY (enable-protected), ENABLE, SWRST (write-sync: ENABLE, SWRST) |
| 0x01 | CTRLB | START (write 1 to start measurement; no SYNCBUSY) |
| 0x02 | CTRLC | FREERUN, WINMODE[2:0] (enable-protected) |
| 0x03 | EVCTRL | WINEO, STARTINV, STARTEI (enable-protected) |
| 0x04 | INTENCLR | OVF, WINMON, OVERRUN, RESRDY |
| 0x05 | INTENSET | (same) |
| 0x06 | INTFLAG | (same — write 1 or read VALUE to clear RESRDY/WINMON) |
| 0x07 | STATUS | OVF (read-only; 1=result invalid) |
| 0x08 | SYNCBUSY | ENABLE, SWRST (32-bit read-only) |
| 0x0C | VALUE | VALUE[23:0] (24-bit signed, read-only) |
| 0x10 | WINLT | WINLT[23:0] (24-bit signed, enable-protected) |
| 0x14 | WINUT | WINUT[23:0] (24-bit signed, enable-protected) |
| 0x18 | GAIN | GAIN[23:0] (enable-protected; NOT reset by SWRST; load from NVM) |
| 0x1C | OFFSET | OFFSETC[23:0] (enable-protected; NOT reset by SWRST; load from NVM) |
| 0x20 | CAL | FCAL[5:0], TCAL[5:0] (enable-protected; load from NVM) |
| 0x24 | DBGCTRL | DBGRUN |

## See Also

- [[NVMCTRL]] — NVM Temperature Calibration Area (NVMCTRL_TEMP_LOG, section 9.4)
- [[Clock System]] — GCLK_TSENS setup (OSC48M recommended for calibrated accuracy)
- [[EVSYS]] — event-triggered TSENS measurements
- [[DMA Controller]] — TSENS result DMA transfer
