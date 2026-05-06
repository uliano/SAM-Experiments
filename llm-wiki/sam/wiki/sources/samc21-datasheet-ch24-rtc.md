---
title: SAMC21 Datasheet Chapter 24 — RTC
type: source
tags: [rtc, timer, calendar, alarm, samc21, datasheet]
date: 2026-05-05
---

# SAMC21 Datasheet Ch. 24 — RTC – Real-Time Counter

Source: SAM C20/C21 Data Sheet DS60001479D, chapter 24, pages 279–327.

## Overview

The Real-Time Counter (RTC) is a 32-bit counter driven by a low-power clock source
independent of the main CPU clock. Three operating modes are supported:
- **MODE 0** (COUNT32): 32-bit free-running counter with one compare register.
- **MODE 1** (COUNT16): 16-bit counter with programmable period and two compares.
- **MODE 2** (CLOCK/Calendar): packed date+time register with one alarm.

The RTC is on the APBA bus (not APBB). It resets only on POR or SWRST — not on
WDT, EXT, or SYST resets.

## Clock Sources

Configured via OSC32KCTRL.RTCCTRL.RTCSEL:

| RTCSEL | Source | Frequency |
|--------|--------|-----------|
| 0x0 | ULP1K | OSCULP32K / 1024 ≈ 1 kHz |
| 0x1 | ULP32K | OSCULP32K ≈ 32.768 kHz |
| 0x4 | XOSC1K | XOSC32K / 1024 = 1024 Hz (exact) |
| 0x5 | XOSC32K | XOSC32K = 32768 Hz (exact) |
| 0x6 | GCLK_RTC | User-defined GCLK generator |

For accurate calendar (1 Hz tick): use XOSC1K + PRESCALER=DIV1024.
OSCULP32K-derived sources are approximate (~1% accuracy).

## CTRLA Fields (All Modes)

| Field | Bit(s) | Description |
|-------|--------|-------------|
| SWRST | 0 | Software reset (write-synchronized) |
| ENABLE | 1 | Enable (write-synchronized) |
| MODE[1:0] | 3:2 | 0=COUNT32, 1=COUNT16, 2=CLOCK |
| PRESCALER[3:0] | 11:8 | Clock prescaler: 0=OFF, 1=DIV1 … 0xB=DIV1024 |
| BKTRST | 13 | GPBKUP reset on RTC reset |
| GPTRST | 14 | GPBKUP reset on tamper |
| COUNTSYNC (MODE 0/1) | 15 | Enable read synchronization of COUNT |
| CLOCKSYNC (MODE 2) | 15 | Enable read synchronization of CLOCK |

**MATCHCLR (bit 7, MODE 2 only):** Clear CLOCK register on alarm match.
**CLKREP (bit 6, MODE 2 only):** 0 = 24-hour, 1 = 12-hour format.

### PRESCALER Values

| Value | Divisor | With 1024 Hz input → counter rate |
|-------|---------|-----------------------------------|
| 0x0 | OFF | — (no counter output) |
| 0x1 | 1 | 1024 Hz |
| 0x2 | 2 | 512 Hz |
| 0x3 | 4 | 256 Hz |
| 0x4 | 8 | 128 Hz |
| 0x5 | 16 | 64 Hz |
| 0x6 | 32 | 32 Hz |
| 0x7 | 64 | 16 Hz |
| 0x8 | 128 | 8 Hz |
| 0x9 | 256 | 4 Hz |
| 0xA | 512 | 2 Hz |
| 0xB | 1024 | 1 Hz |

## EVCTRL Register (0x04, 32-bit)

| Field | Bits | Description |
|-------|------|-------------|
| PEREO[7:0] | 7:0 | Periodic interval event output enable |
| ALARMEO0 | 8 | Alarm 0 event output enable |
| TAMPEREO | 14 | Tamper event output enable |
| OVFEO | 15 | Overflow event output enable |

## Interrupt Flags (common across modes)

INTENCLR (0x08), INTENSET (0x0A), INTFLAG (0x0C) — all 16-bit, w1c for INTFLAG:

| Bit | Name | Trigger |
|-----|------|---------|
| 15 | OVF | Counter overflow |
| 8 | ALARM0 | Compare/alarm match |
| 7:0 | PER7:PER0 | Prescaler bit [n+2] 0→1 transition |

PERn fires at CLK_RTC / 2^(n+2). With 1024 Hz input:
- PER0: 256 Hz, PER4: 16 Hz, PER7: 2 Hz.

## DBGCTRL (0x0E)

DBGRUN(bit 0): 0 = RTC halts when CPU halted by debugger; 1 = RTC continues.
Not reset by software reset.

## FREQCORR (0x14, Write-Synchronized)

| Field | Bit(s) | Description |
|-------|--------|-------------|
| SIGN | 7 | 0 = positive correction (slow down); 1 = negative (speed up) |
| VALUE[6:0] | 6:0 | 0 = disabled; 1–127 = correction steps |

## Mode 0 / Mode 1 Registers

### Mode 0 (COUNT32)

| Offset | Register | Description |
|--------|----------|-------------|
| 0x10 | SYNCBUSY | COUNTSYNC(15)/COMP0(5)/COUNT(3)/FREQCORR(2)/ENABLE(1)/SWRST(0) |
| 0x18 | COUNT (32-bit) | Read-synchronized when COUNTSYNC=1; use READSYNC cmd otherwise |
| 0x20 | COMP0 (32-bit) | Compare value; alarm when COUNT == COMP0 |

### Mode 1 (COUNT16)

| Offset | Register | Description |
|--------|----------|-------------|
| 0x10 | SYNCBUSY | COUNTSYNC(15)/COMP1(6)/COMP0(5)/PER(3)/COUNT(2)/FREQCORR(1)/ENABLE(0)... |
| 0x18 | COUNT (16-bit) | 16-bit counter |
| 0x1C | PER (16-bit) | Period; counter wraps at PER (not 0xFFFF) |
| 0x20 | COMP0 (16-bit) | Compare 0 |
| 0x22 | COMP1 (16-bit) | Compare 1 |

**READSYNC command (0x4):** Must be issued to CTRLA before reading COUNT when
COUNTSYNC=0. Wait SYNCBUSY.COUNT=0 after issuing.

## Mode 2 (CLOCK/Calendar) Registers

### SYNCBUSY (0x10, Mode 2)

| Bit | Field | Description |
|-----|-------|-------------|
| 15 | CLOCKSYNC | CTRLA.CLOCKSYNC write sync |
| 11 | MASK0 | MASK register write sync |
| 5 | ALARM0 | ALARM register write sync |
| 3 | CLOCK | CLOCK register read/write sync |
| 2 | FREQCORR | FREQCORR write sync |
| 1 | ENABLE | CTRLA.ENABLE write sync |
| 0 | SWRST | CTRLA.SWRST write sync |

### CLOCK Register (0x18, 32-bit, Write+Read-Synchronized)

| Bits | Field | Range | Notes |
|------|-------|-------|-------|
| 31:26 | YEAR[5:0] | 0–63 | Offset from firmware-defined reference year; YEAR[1:0]=0 → leap year |
| 25:22 | MONTH[3:0] | 1–12 | 1=January, 12=December |
| 21:17 | DAY[4:0] | 1–31 | Month-dependent maximum |
| 16:12 | HOUR[4:0] | 0–23 (24h) | CLKREP=1: HOUR[3:0]=1–12, HOUR[4]=AM(0)/PM(1) |
| 11:6 | MINUTE[5:0] | 0–59 | |
| 5:0 | SECOND[5:0] | 0–59 | |

Must enable CTRLA.CLOCKSYNC=1 and wait SYNCBUSY.CLOCKSYNC=0 before reading CLOCK.
Write: wait SYNCBUSY.CLOCK=0 before each write.

### ALARM Register (0x20, 32-bit, Write-Synchronized)

Same bit-field layout as CLOCK. Comparison is masked by MASK.SEL.
ALARM is write-synchronized — wait SYNCBUSY.ALARM0=0 after writing.

### MASK Register (0x24, 8-bit, Write-Synchronized)

SEL[2:0] defines which fields of ALARM are matched:

| SEL | Name | Fields compared |
|-----|------|-----------------|
| 0x0 | OFF | Alarm disabled |
| 0x1 | SS | SECOND only |
| 0x2 | MMSS | SECOND + MINUTE |
| 0x3 | HHMMSS | SECOND + MINUTE + HOUR |
| 0x4 | DDHHMMSS | + DAY |
| 0x5 | MMDDHHMMSS | + MONTH |
| 0x6 | YYMMDDHHMMSS | + YEAR (full match) |
| 0x7 | — | Reserved |

## Key Facts

- RTC is on APBA: `MCLK->APBAMASK.reg |= MCLK_APBAMASK_RTC`.
- RTC resets on POR and SWRST only — persists through WDT/EXT/SYST resets.
- All writable registers (except CTRLA/CTRLB) require PAC write protection to be
  cleared before writing; INTFLAG does not require PAC.
- Write-synchronized registers: must wait SYNCBUSY before each write.
- COUNT (Mode 0/1): either set COUNTSYNC=1 (adds latency) or issue READSYNC cmd.
- CLOCK (Mode 2): set CTRLA.CLOCKSYNC=1, wait SYNCBUSY.CLOCKSYNC=0, then read.
- MATCHCLR=1 with MODE=2: CLOCK resets to 0 on alarm match (unusual, not for calendar).
- FREQCORR.SIGN=0 slows the RTC (reduces frequency); SIGN=1 speeds it up.
- DBGRUN=0 (reset default): RTC halts on debugger halt — keeps timestamps frozen.
- DBGRUN is not cleared by SWRST.

## See Also

- [[RTC]] — firmware initialization patterns for all three modes
- [[OSC32KCTRL]] — RTCCTRL clock source selection
- [[Clock System]] — GCLK_RTC routing
- [[NVIC Interrupt Map]] — RTC IRQn value
