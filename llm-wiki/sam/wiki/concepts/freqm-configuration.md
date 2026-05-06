---
title: FREQM Configuration
type: concept
tags: [freqm, frequency, clock, firmware, samc21]
sources: [samc21-datasheet-ch44-freqm]
created: 2026-05-05
updated: 2026-05-05
---

# FREQM Configuration

Frequency Meter on the SAM C20/C21. Measures one clock frequency against a
slower reference clock with 24-bit resolution.

## Basic Init — Measure GCLK0 (48 MHz) against OSCULP32K (32 kHz)

```cpp
// Measure 48 MHz GCLK0 using OSCULP32K as reference.
// With REFNUM=100: f_msr = (VALUE / 100) × 32768 Hz
// Expected VALUE ≈ 100 × 48000000 / 32768 ≈ 146484

void freqm_init(void) {
    // 1. Configure GCLK reference: PCHCTRL[4] = GCLK_FREQM_REF
    //    Use a generator sourced from OSCULP32K (e.g. GCLK generator 3 = OSCULP32K)
    GCLK->PCHCTRL[4].reg = GCLK_PCHCTRL_GEN_GCLK3 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[4].reg & GCLK_PCHCTRL_CHEN));

    // 2. Configure GCLK measured: PCHCTRL[3] = GCLK_FREQM_MSR
    //    Use GCLK0 = 48 MHz (the clock to be measured)
    GCLK->PCHCTRL[3].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[3].reg & GCLK_PCHCTRL_CHEN));

    // 3. No MCLK mask needed — FREQM APB clock is enabled at reset (APB-A)

    // 4. Reset
    FREQM->CTRLA.reg = FREQM_CTRLA_SWRST;
    while (FREQM->SYNCBUSY.reg & FREQM_SYNCBUSY_SWRST);

    // 5. Configure (enable-protected: write before ENABLE)
    FREQM->CFGA.reg = FREQM_CFGA_REFNUM(100);

    // 6. Enable
    FREQM->CTRLA.reg |= FREQM_CTRLA_ENABLE;
    while (FREQM->SYNCBUSY.reg & FREQM_SYNCBUSY_ENABLE);
}
```

## Measurement — Polling

```cpp
uint32_t freqm_measure_hz(uint32_t f_ref_hz, uint8_t refnum) {
    // Start measurement
    FREQM->CTRLB.reg = FREQM_CTRLB_START;

    // Wait for completion (poll BUSY)
    while (FREQM->STATUS.reg & FREQM_STATUS_BUSY);

    // Check overflow
    if (FREQM->STATUS.reg & FREQM_STATUS_OVF) {
        FREQM->STATUS.reg = FREQM_STATUS_OVF;  // clear sticky flag
        return 0;  // invalid — reduce REFNUM or use slower reference
    }

    uint32_t value = FREQM->VALUE.reg & 0xFFFFFF;
    // f_msr = (VALUE / REFNUM) × f_ref
    // Use 64-bit intermediate to avoid overflow at high frequencies:
    return (uint32_t)((uint64_t)value * f_ref_hz / refnum);
}
```

## Measurement — DONE Interrupt

```cpp
void FREQM_Handler(void) {
    if (FREQM->INTFLAG.reg & FREQM_INTFLAG_DONE) {
        FREQM->INTFLAG.reg = FREQM_INTFLAG_DONE;  // clear flag

        if (FREQM->STATUS.reg & FREQM_STATUS_OVF) {
            FREQM->STATUS.reg = FREQM_STATUS_OVF;
            // result invalid; adjust REFNUM
        } else {
            uint32_t val = FREQM->VALUE.reg & 0xFFFFFF;
            // f_msr = (val / REFNUM) × f_ref
        }
    }
}
```

## Key Facts

- Formula: `f_CLK_MSR = (VALUE / REFNUM) × f_CLK_REF`
- **Reference clock must be slower than the measured clock.**
- GCLK PCHCTRL[3] = measurement clock (GCLK_FREQM_MSR).
- GCLK PCHCTRL[4] = reference clock (GCLK_FREQM_REF).
- FREQM APB clock is enabled at reset (APB-A bridge) — no MCLK mask needed.
- CFGA.REFNUM is enable-protected: write before CTRLA.ENABLE=1.
- CTRLA.ENABLE and CTRLA.SWRST are write-synchronized; poll SYNCBUSY.
- CTRLB.START: write-only, no SYNCBUSY.
- STATUS.OVF is sticky: cleared by writing 1 to OVF (not by starting another measurement).
- DIVREF (CFGA bit 15): divide reference by 8 (N-series only) — allows using a faster reference source.
- VALUE is 24-bit unsigned; max count = 16777215.
- No DMA, no events output.
- NVIC line 4.

## Frequency Resolution and REFNUM Selection

| f_ref | REFNUM | Resolution | Max measurable |
|-------|--------|-----------|----------------|
| 32.768 kHz | 100 | ~327 Hz | ~5.49 GHz* |
| 32.768 kHz | 255 | ~128 Hz | ~2.15 GHz* |
| 1 MHz | 100 | 10 kHz | ~167 GHz* |

*Limited by VALUE overflow at 24 bits; OVF flag set if exceeded.

Higher REFNUM → better resolution but longer measurement time.
Measurement duration = REFNUM / f_ref (e.g. 100/32768 ≈ 3 ms).

## See Also

- [[SAMC21 Datasheet Ch.44 FREQM]] — full register reference
- [[Clock System]] — GCLK generator and PCHCTRL setup
- [[NVIC Interrupt Map]] — FREQM on NVIC line 4
- [[OSCCTRL]] — XOSC_FAIL detection via FREQM comparison
