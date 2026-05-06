---
title: SAMC21 Datasheet Chapter 16 — GCLK
type: source
tags: [gclk, clock, samc21, datasheet]
date: 2026-05-05
---

# SAMC21 Datasheet Ch. 16 — GCLK Generic Clock Controller

Source: SAM C20/C21 Data Sheet DS60001479D, chapter 16, pages 137–152.

## Overview

The GCLK module has:
- 9 Generic Clock Generators (0–8)
- Up to 46 Peripheral Channels (m = 0–45)
- Base address: 0x40001C00 (APB-A)

Generator 0 = GCLK_MAIN, always feeds the MCLK synchronous clock controller.
Generator 1 can be used as source for other generators (the only one that can).

## Reset State (after power-on)

Generator 0 resets to 0x00000106: GENEN=1, SRC=0x06 (OSC48M), DIV=0. All others reset to 0.
After reset: OSC48M enabled and divided by 12 → GCLK_MAIN = 4 MHz at reset.

## Register Map

| Offset | Name | Width | Description |
|--------|------|-------|-------------|
| 0x00 | CTRLA | 8 | Control A (SWRST bit 0) |
| 0x04 | SYNCBUSY | 32 | Synchronization Busy |
| 0x20 + n×4 | GENCTRLn | 32 | Generator n Control (n=0..8) |
| 0x80 + m×4 | PCHCTRLm | 32 | Peripheral Channel m Control (m=0..45) |

Last PCHCTRLm: PCHCTRL45 at offset 0x134.

## GENCTRLn Register (offset 0x20 + n×4)

32-bit, Write-Synchronized, PAC Write-Protected.

| Bits | Name | Description |
|------|------|-------------|
| 31:24 | DIV[15:8] | Division factor high byte (only Gen 1 uses all 16 bits) |
| 23:16 | DIV[7:0] | Division factor low byte |
| 13 | RUNSTDBY | Run in Standby |
| 12 | DIVSEL | 0 = divide by DIV; 1 = divide by 2^(DIV+1) |
| 11 | OE | Output clock to GCLK_IO pin |
| 10 | OOV | Output Off Value (pin level when OE=0 or generator off) |
| 9 | IDC | Improve Duty Cycle (50/50 for odd divisors) |
| 8 | GENEN | Generator Enable |
| 4:0 | SRC[4:0] | Clock Source Selection (see table below) |

**Important:** Write the full 32-bit word to GENCTRLn in one atomic access. Partial writes cause errors due to Write-Synchronized property.

Division factor bits available:
- Generator 0: DIV[7:0] (8 bits)
- Generator 1: DIV[15:0] (16 bits)
- Generators 2–8: DIV[7:0] (8 bits)

If DIVSEL=0 and DIV=0 or 1: output = source (undivided).

### SRC Values

| Value | Name | Source |
|-------|------|--------|
| 0x00 | XOSC | External crystal oscillator |
| 0x01 | GCLK_IN | External clock from GCLK_IO pin |
| 0x02 | GCLK_GEN1 | Generator 1 output |
| 0x03 | OSCULP32K | Ultra-low-power 32K oscillator |
| 0x04 | OSC32K | 32K internal oscillator |
| 0x05 | XOSC32K | External 32K crystal |
| 0x06 | OSC48M | 48 MHz internal oscillator |
| 0x07 | DPLL96M | FDPLL96M output |

### Reset Values

| Generator | After Power Reset |
|-----------|------------------|
| 0 | 0x00000106 (GENEN=1, SRC=OSC48M) |
| 1–8 | 0x00000000 |

After user reset: Gen 0 = 0x00000106; others unchanged if their PCHCTRLm.WRTLOCK=1, else 0.

## PCHCTRLm Register (offset 0x80 + m×4)

8-bit significant, PAC Write-Protected.

| Bits | Name | Description |
|------|------|-------------|
| 7 | WRTLOCK | Write Lock — locks this register and its generator until power reset |
| 6 | CHEN | Channel Enable (write-synchronized, read-back delayed) |
| 3:0 | GEN[3:0] | Generator selection (0–8) |

To change the clock source of an enabled peripheral channel without glitches:
1. Write CHEN=0
2. Poll until CHEN reads 0
3. Write new GEN
4. Write CHEN=1

## SYNCBUSY Register (offset 0x04)

| Bits | Description |
|------|-------------|
| 10 | GENCTRL8 sync busy |
| 9 | GENCTRL7 sync busy |
| 8 | GENCTRL6 sync busy |
| 7 | GENCTRL5 sync busy |
| 6 | GENCTRL4 sync busy |
| 5 | GENCTRL3 sync busy |
| 4 | GENCTRL2 sync busy |
| 3 | GENCTRL1 sync busy |
| 2 | GENCTRL0 sync busy |
| 0 | SWRST sync busy |

## PCHCTRLm Mapping — Complete Table

| m | Name | Peripheral |
|---|------|-----------|
| 0 | GCLK_DPLL | FDPLL96M reference clock |
| 1 | GCLK_DPLL_32K | FDPLL96M 32kHz internal timer |
| 2 | GCLK_EIC | External Interrupt Controller |
| 3 | GCLK_FREQM_MSR | FREQM measure input |
| 4 | GCLK_FREQM_REF | FREQM reference |
| 5 | GCLK_TSENS | Temperature Sensor |
| 6 | GCLK_EVSYS_CH0 | Event System channel 0 |
| 7 | GCLK_EVSYS_CH1 | Event System channel 1 |
| 8 | GCLK_EVSYS_CH2 | Event System channel 2 |
| 9 | GCLK_EVSYS_CH3 | Event System channel 3 |
| 10 | GCLK_EVSYS_CH4 | Event System channel 4 |
| 11 | GCLK_EVSYS_CH5 | Event System channel 5 |
| 12 | GCLK_EVSYS_CH6 | Event System channel 6 |
| 13 | GCLK_EVSYS_CH7 | Event System channel 7 |
| 14 | GCLK_EVSYS_CH8 | Event System channel 8 |
| 15 | GCLK_EVSYS_CH9 | Event System channel 9 |
| 16 | GCLK_EVSYS_CH10 | Event System channel 10 |
| 17 | GCLK_EVSYS_CH11 | Event System channel 11 |
| 18 | GCLK_SERCOM[0-4]_SLOW | SERCOM 0–4 slow clock (used for USART) |
| 19 | GCLK_SERCOM0_CORE | SERCOM0 core |
| 20 | GCLK_SERCOM1_CORE | SERCOM1 core |
| 21 | GCLK_SERCOM2_CORE | SERCOM2 core |
| 22 | GCLK_SERCOM3_CORE | SERCOM3 core |
| 23 | GCLK_SERCOM4_CORE | SERCOM4 core |
| 24 | GCLK_SERCOM5_SLOW | SERCOM5 slow clock |
| 25 | GCLK_SERCOM5_CORE | SERCOM5 core |
| 26 | GCLK_TCC0_TCC1 | TCC0 and TCC1 |
| 27 | GCLK_TCC2 | TCC2 |
| 28 | GCLK_TC0_TC1 | TC0 and TC1 |
| 29 | GCLK_TC2_TC3 | TC2 and TC3 |
| 30 | GCLK_TC4 | TC4 |
| 31 | GCLK_ADC0 | ADC0 |
| 32 | GCLK_ADC1 | ADC1 |
| 33 | GCLK_SDADC | SDADC |
| 34 | GCLK_AC | Analog Comparator |
| 35 | GCLK_DAC | DAC |
| 36 | GCLK_PTC | Peripheral Touch Controller |
| 37 | GCLK_CCL | Configurable Custom Logic |
| 38 | GCLK_CAN0 | CAN0 |
| 39 | GCLK_CAN1 | CAN1 |

Note: indices 19 onwards are from the continuation of the PCHCTRLm mapping table (not shown in pages 151–152 above). The firmware uses `TC0_GCLK_ID` / `SERCOM5_GCLK_ID_CORE` constants from the CMSIS headers.

## Typical Generator Setup

```cpp
// Configure GCLK generator 3 from OSC48M, divide by 48 → 1 MHz
GCLK->GENCTRL[3].reg =
    GCLK_GENCTRL_SRC_OSC48M |
    GCLK_GENCTRL_DIV(48)    |
    GCLK_GENCTRL_GENEN;
while (GCLK->SYNCBUSY.bit.GENCTRL3);

// Connect generator 3 to TC0
GCLK->PCHCTRL[TC0_GCLK_ID].reg =
    GCLK_PCHCTRL_GEN_GCLK3 |
    GCLK_PCHCTRL_CHEN;
while (!(GCLK->PCHCTRL[TC0_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));
```

## See Also

- [[Clock System]] — three-layer clock architecture overview
- [[OSCCTRL]] — oscillator configuration (OSC48M, XOSC, DPLL)
- [[MCLK]] — synchronous clock controller (CPUDIV, AHB/APB masks)
- [[TC 32-Bit Paired Mode]] — uses TC0_GCLK_ID / TC2_GCLK_ID
- [[SERCOM UART Configuration]] — uses SERCOM5_GCLK_ID_CORE
