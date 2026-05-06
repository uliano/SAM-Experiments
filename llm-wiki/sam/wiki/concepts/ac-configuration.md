---
title: AC Configuration
type: concept
tags: [ac, comparator, analog, firmware, samc21]
sources: [samc21-datasheet-ch40-ac]
created: 2026-05-05
updated: 2026-05-05
---

# AC Configuration

Analog Comparator (AC) on the SAM C21: four comparators in two pairs.
Each comparator is independently configurable for continuous or single-shot
operation, with optional digital filter, hysteresis, window mode, and VDD scaler.

## Basic Init — COMP0, Continuous, AIN0 vs VDD/2

```cpp
// COMP0: AIN0 positive, VDD scaler negative (VDD/2 = 64 levels → VALUE=31)
// Continuous mode, filter 3-bit majority, interrupt on toggle

void ac_init(void) {
    // 1. Clocks
    MCLK->APBBMASK.reg |= MCLK_APBBMASK_AC;
    GCLK->PCHCTRL[AC_GCLK_ID].reg =
        GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[AC_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));

    // 2. Reset
    AC->CTRLA.reg = AC_CTRLA_SWRST;
    while (AC->SYNCBUSY.reg & AC_SYNCBUSY_SWRST);

    // 3. VDD scaler: VALUE=31 → (31+1)/64 × VDD = VDD/2
    AC->SCALER[0].reg = 31;

    // 4. Configure COMP0 (enable-protected per bit, but COMPCTRL not enable-protected)
    AC->COMPCTRL[0].reg =
        AC_COMPCTRL_MUXPOS_PIN0       // AIN0 as positive input
        | AC_COMPCTRL_MUXNEG_VSCALE   // VDD scaler as negative input
        | AC_COMPCTRL_SPEED_HIGH       // fastest response
        | AC_COMPCTRL_HYSTEN           // hysteresis (continuous mode only)
        | AC_COMPCTRL_FLEN_MAJ3        // 3-bit majority filter
        | AC_COMPCTRL_INTSEL_TOGGLE    // interrupt on any edge
        | AC_COMPCTRL_OUT_SYNC;        // sync output to GCLK_AC
    while (AC->SYNCBUSY.reg & AC_SYNCBUSY_COMPCTRL0);

    // 5. Enable interrupt
    AC->INTENSET.reg = AC_INTENSET_COMP0;
    NVIC_EnableIRQ(AC_IRQn);

    // 6. Enable AC global and COMP0
    AC->CTRLA.reg |= AC_CTRLA_ENABLE;
    while (AC->SYNCBUSY.reg & AC_SYNCBUSY_ENABLE);

    AC->COMPCTRL[0].reg |= AC_COMPCTRL_ENABLE;
    while (AC->SYNCBUSY.reg & AC_SYNCBUSY_COMPCTRL0);
}

void AC_Handler(void) {
    if (AC->INTFLAG.reg & AC_INTFLAG_COMP0) {
        AC->INTFLAG.reg = AC_INTFLAG_COMP0;
        bool above = (AC->STATUSA.reg & AC_STATUSA_STATE0) != 0;
        // act on comparator result
    }
}
```

## Single-Shot Mode

```cpp
// COMP1 single-shot triggered by software
// Result available in STATUSA.STATE1 after INTFLAG.COMP1 fires

AC->COMPCTRL[1].reg =
    AC_COMPCTRL_MUXPOS_PIN2         // AIN2 as positive input
    | AC_COMPCTRL_MUXNEG_DAC        // DAC output as reference
    | AC_COMPCTRL_SINGLE            // single-shot mode
    | AC_COMPCTRL_INTSEL_EOC        // interrupt at end of comparison
    | AC_COMPCTRL_SPEED_HIGH;
while (AC->SYNCBUSY.reg & AC_SYNCBUSY_COMPCTRL1);

AC->COMPCTRL[1].reg |= AC_COMPCTRL_ENABLE;
while (AC->SYNCBUSY.reg & AC_SYNCBUSY_COMPCTRL1);

// Trigger comparison:
AC->CTRLB.reg = AC_CTRLB_START1;

// Wait or use interrupt:
while (!(AC->INTFLAG.reg & AC_INTFLAG_COMP1));
AC->INTFLAG.reg = AC_INTFLAG_COMP1;
bool result = (AC->STATUSA.reg & AC_STATUSA_STATE1) != 0;
```

## Window Mode (COMP0 + COMP1 as pair)

```cpp
// Window: AIN0 (shared positive input), low threshold on COMP1, high on COMP0
// WSTATE: ABOVE(0)/INSIDE(1)/BELOW(2) in STATUSA.WSTATE0

// VDD scaler for thresholds
AC->SCALER[0].reg = 40;   // high threshold: 41/64 × VDD
AC->SCALER[1].reg = 20;   // low threshold:  21/64 × VDD

AC->COMPCTRL[0].reg =
    AC_COMPCTRL_MUXPOS_PIN0 | AC_COMPCTRL_MUXNEG_VSCALE
    | AC_COMPCTRL_INTSEL_TOGGLE | AC_COMPCTRL_ENABLE;
while (AC->SYNCBUSY.reg & AC_SYNCBUSY_COMPCTRL0);

AC->COMPCTRL[1].reg =
    AC_COMPCTRL_MUXPOS_PIN0 | AC_COMPCTRL_MUXNEG_VSCALE
    | AC_COMPCTRL_INTSEL_TOGGLE | AC_COMPCTRL_ENABLE;
while (AC->SYNCBUSY.reg & AC_SYNCBUSY_COMPCTRL1);

AC->WINCTRL.reg = AC_WINCTRL_WEN0;
while (AC->SYNCBUSY.reg & AC_SYNCBUSY_WINCTRL);

AC->INTENSET.reg = AC_INTENSET_WIN0;
```

## Wakeup from STANDBY

```cpp
// COMP0 wakes the CPU from STANDBY when AIN0 exceeds threshold
AC->COMPCTRL[0].reg |= AC_COMPCTRL_RUNSTDBY;
while (AC->SYNCBUSY.reg & AC_SYNCBUSY_COMPCTRL0);
// No GCLK_AC needed for async wakeup in continuous mode
```

## MUXPOS / MUXNEG Reference

| MUXPOS value | Input |
|--------------|-------|
| 0x0 (PIN0) | AIN0 |
| 0x1 (PIN1) | AIN1 |
| 0x2 (PIN2) | AIN2 |
| 0x3 (PIN3) | AIN3 |
| 0x4 (VSCALE) | VDD scaler |

| MUXNEG value | Input |
|--------------|-------|
| 0x0 (PIN0) | AIN0 |
| 0x1 (PIN1) | AIN1 |
| 0x2 (PIN2) | AIN2 |
| 0x3 (PIN3) | AIN3 |
| 0x4 (GND) | Ground |
| 0x5 (VSCALE) | VDD scaler |
| 0x6 (BANDGAP) | Internal bandgap |
| 0x7 (DAC) | DAC output |

## Key Facts

- Four comparators: COMP0/COMP1 (pair 0), COMP2/COMP3 (pair 1).
- Only EVCTRL is enable-protected; COMPCTRLn and other config registers are not.
- Write-synchronized: CTRLA.SWRST/ENABLE, COMPCTRLn.ENABLE, WINCTRL.
- Hysteresis only works in continuous mode (SINGLE=0).
- FLEN filter delays result by N−1 GCLK_AC cycles. No filter in single-shot mode.
- SWAP swaps MUXPOS/MUXNEG for offset compensation; use only while comparator disabled.
- STATUSB.READYx: wait for this before reading STATUSA.STATEx in continuous mode.
- VDD scaler 64 levels: V_out = V_DD × (VALUE+1)/64.
- Window WSTATE0/1: ABOVE=0x0, INSIDE=0x1, BELOW=0x2.
- Continuous mode with RUNSTDBY: asynchronous detection in STANDBY (no GCLK needed).

## See Also

- [[SAMC21 Datasheet Ch.40 AC]] — full register reference
- [[EIC]] — digital edge/level interrupt alternative
- [[EVSYS]] — event-triggered single-shot comparisons
- [[SUPC]] — supply voltage and bandgap reference
- [[Clock System]] — GCLK_AC setup
