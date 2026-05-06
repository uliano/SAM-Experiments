---
title: SAMC21 Datasheet Ch.20 OSCCTRL
type: source
tags: [oscctrl, oscillator, clock, xosc, osc48m, dpll, cfd, samc21, datasheet]
sources: [samc21-datasheet-ch20-oscctrl]
created: 2026-05-05
updated: 2026-05-05
---

# SAMC21 Datasheet Ch.20 OSCCTRL — Oscillators Controller

Oscillators Controller for the SAM C20/C21. Manages the XOSC external crystal
oscillator, the OSC48M internal 48 MHz oscillator, and the FDPLL96M fractional
PLL (48–96 MHz output range).

## Abstract

OSCCTRL provides three clock sources feeding the GCLK layer. XOSC accepts a
0.4–32 MHz external crystal or clock. OSC48M is the always-on internal RC
oscillator enabled at reset (divided by 12 = 4 MHz default). FDPLL96M can
synthesize any frequency in the 48–96 MHz range from a crystal or GCLK
reference. Clock Failure Detection (CFD) monitors XOSC and switches to an OSC48M-derived safe clock on failure.

## Key Facts

- Base address: 0x40001000 (APB-A, enabled by default — no MCLK mask needed).
- OSC48M enabled at reset with DIV=0xB (÷12 → 4 MHz). Write DIV=0 for 48 MHz.
- OSC48MDIV is write-synchronized: poll OSC48MSYNCBUSY.OSC48MDIV after writing.
- XOSCCTRL.GAIN must be set to match crystal frequency even when AMPGC=1.
  - GAIN=0x0: ≤2 MHz; 0x1: ≤4 MHz; 0x2: ≤8 MHz; 0x3: ≤16 MHz; 0x4: ≤30 MHz.
- XOSCCTRL.STARTUP[3:0]: startup counter in OSCULP32K cycles (0x0=1 cycle/31μs → 0xF=32768 cycles/1000ms).
- CFD: XOSCCTRL.CFDEN=1 enables detector; safe clock = OSC48M / 2^CFDPRESC.
  STATUS.CLKSW=1 and STATUS.CLKFAIL=1 when failure detected.
  XOSCCTRL.SWBEN=1 enables auto-switch-back on XOSC recovery.
- DPLLCTRLB (DPLL Control B) is enable-protected: must be written before DPLLCTRLA.ENABLE=1.
- DPLLCTRLA.ENABLE is write-synchronized: poll DPLLSYNCBUSY.ENABLE after writing.
- DPLLRATIO is write-synchronized: poll DPLLSYNCBUSY.DPLLRATIO after writing.
- DPLL output formula: `f_CK = f_CKR × (LDR + 1 + LDRFRAC/16) / 2^PRESC`
  where f_CKR is the selected reference clock (optionally pre-divided by DIV for XOSC reference).
- DPLL reference clock (DPLLCTRLB.REFCLK): 0x0=XOSC32K, 0x1=XOSC, 0x2=GCLK.
- CAL48M (offset 0x38): calibration register; only valid on Rev D silicon. Load from NVM Software Calibration Area (FCAL, FRANGE, TCAL fields).
- OSCCTRL shares NVIC line 0 with PM, MCLK, OSC32KCTRL, SUPC, PAC.

## Register Summary

| Offset | Name | Key Fields |
|--------|------|-----------|
| 0x00 | INTENCLR | DPLLLDRTO(11), DPLLLTO(10), DPLLLCKF(9), DPLLLCKR(8), OSC48MRDY(4), CLKFAIL(1), XOSCRDY(0) — PAC Write-Protected |
| 0x04 | INTENSET | Same bits — PAC Write-Protected |
| 0x08 | INTFLAG | Same bits — cleared by writing 1 |
| 0x0C | STATUS | DPLLLDRTO(11), DPLLLTO(10), DPLLLCKF(9), DPLLLCKR(8), OSC48MRDY(4), CLKSW(2), CLKFAIL(1), XOSCRDY(0) — read-only |
| 0x10 | XOSCCTRL | STARTUP[15:12], AMPGC(11), GAIN[10:8], ONDEMAND(7), RUNSTDBY(6), SWBEN(4), CFDEN(3), XTALEN(2), ENABLE(1) — PAC Write-Protected, 16-bit |
| 0x12 | CFDPRESC | CFDPRESC[2:0] — PAC Write-Protected, 8-bit |
| 0x13 | EVCTRL | CFDEO(0) — PAC Write-Protected, 8-bit; event output on CFD |
| 0x14 | OSC48MCTRL | ONDEMAND(7), RUNSTDBY(6), ENABLE(1) — PAC Write-Protected, 8-bit; Reset=0x82 |
| 0x15 | OSC48MDIV | DIV[3:0] — 8-bit; f=48/(DIV+1); Reset=0x0B (4 MHz) |
| 0x16 | OSC48MSTUP | STARTUP[2:0] — 8-bit startup delay; Reset=0x07 (1024 cycles/21.3 μs) |
| 0x18 | OSC48MSYNCBUSY | OSC48MDIV(2) — 32-bit read-only |
| 0x1C | DPLLCTRLA | ONDEMAND(7), RUNSTDBY(6), ENABLE(1) — PAC Write-Protected, Write-Synchronized(ENABLE); Reset=0x80 |
| 0x20 | DPLLRATIO | LDRFRAC[19:16], LDR[11:0] — PAC Write-Protected, Write-Synchronized |
| 0x24 | DPLLCTRLB | DIV[26:16], LBYPASS(12), LTIME[10:8], REFCLK[5:4], WUF(3), LPEN(2), FILTER[1:0] — Enable-Protected, PAC Write-Protected |
| 0x28 | DPLLPRESC | PRESC[1:0]: 0=÷1, 1=÷2, 2=÷4 — PAC Write-Protected, Write-Synchronized |
| 0x2C | DPLLSYNCBUSY | DPLLPRESC(3), DPLLRATIO(2), ENABLE(1) — read-only |
| 0x30 | DPLLSTATUS | CLKRDY(1), LOCK(0) — read-only |
| 0x38 | CAL48M | TCAL[21:16], FRANGE[9:8], FCAL[5:0] — PAC Write-Protected; Rev D silicon only |

## DPLLCTRLB Detail

| Bits | Name | Description |
|------|------|-------------|
| 26:16 | DIV[10:0] | Ref clock divider for XOSC ref: f_DIV = f_XOSC / (2×(DIV+1)) |
| 12 | LBYPASS | Lock bypass: 1=always assert lock (skip lock detection) |
| 10:8 | LTIME[2:0] | Lock timeout: 0=none, 0x4=8ms, 0x5=9ms, 0x6=10ms, 0x7=11ms |
| 5:4 | REFCLK[1:0] | 0x0=XOSC32K, 0x1=XOSC, 0x2=GCLK |
| 3 | WUF | Wake Up Fast: 1=output before lock |
| 2 | LPEN | Low Power Enable: 1=disable TDC (lower power, higher jitter) |
| 1:0 | FILTER[1:0] | 0=DEFAULT, 1=LBFILT, 2=HBFILT, 3=HDFILT |

## OSC48M Startup Delay Table

| STARTUP | Cycles | Time |
|---------|--------|------|
| 0x0 | 8 | 166 ns |
| 0x1 | 16 | 333 ns |
| 0x2 | 32 | 667 ns |
| 0x3 | 64 | 1.333 μs |
| 0x4 | 128 | 2.667 μs |
| 0x5 | 256 | 5.333 μs |
| 0x6 | 512 | 10.667 μs |
| 0x7 | 1024 | 21.333 μs ← reset default |

## Concepts Mentioned

- [[OSCCTRL]] — practical init sequences for XOSC, OSC48M, DPLL
- [[Clock System]] — how OSCCTRL feeds the GCLK layer
- [[QuartzTest]] — firmware that exercises XOSC and CFD

## See Also

- [[OSCCTRL]] — concept page with init code
- [[Clock System]] — GCLK generator and PCHCTRL setup
- [[OSC32KCTRL]] — 32 kHz oscillators (XOSC32K, OSC32K, OSCULP32K)
- [[SAMC21 Datasheet Ch.09 Memories]] — NVM Software Calibration Area for CAL48M
