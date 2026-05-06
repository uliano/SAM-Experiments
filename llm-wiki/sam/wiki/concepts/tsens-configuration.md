---
title: TSENS Configuration
type: concept
tags: [tsens, temperature, analog, firmware, samc21]
sources: [samc21-datasheet-ch43-tsens]
created: 2026-05-05
updated: 2026-05-05
---

# TSENS Configuration

Temperature Sensor on the SAM C20/C21. Measures die temperature by comparing
two temperature-dependent oscillator frequencies against GCLK_TSENS. Result is
a 24-bit signed integer: 100 counts = 1°C when using factory NVM calibration
and OSC48M as GCLK_TSENS.

## Basic Init — Single-Shot with RESRDY Interrupt

```cpp
// TSENS: single-shot, GCLK0=48MHz (OSC48M)
// Calibration from NVM Temperature Calibration Area (NVMCTRL_TEMP_LOG)

// NVM Temperature Calibration Area layout (from section 9.4):
// Address 0x00800100, bits as described in datasheet.
// Microchip CMSIS provides NVMCTRL_TEMP_LOG pointer.

void tsens_load_calibration(void) {
    // Read calibration from NVM (using CMSIS-provided macros/pointers)
    uint32_t *nvmtemplog = (uint32_t *)NVMCTRL_TEMP_LOG;

    // GAIN and OFFSET from NVM (24-bit values; exact bit positions per ds section 9.4)
    // These registers are enable-protected and NOT reset by SWRST.
    TSENS->GAIN.reg  = TSENS_FUSES_GAIN(nvmtemplog);    // macro from device header
    TSENS->OFFSET.reg = TSENS_FUSES_OFFSET(nvmtemplog);
    TSENS->CAL.reg   = TSENS_FUSES_FCAL(nvmtemplog)
                     | TSENS_FUSES_TCAL(nvmtemplog);
}

void tsens_init(void) {
    // 1. Clocks
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_TSENS;
    GCLK->PCHCTRL[TSENS_GCLK_ID].reg =
        GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[TSENS_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));

    // 2. Reset (does NOT clear GAIN, OFFSET, CAL)
    TSENS->CTRLA.reg = TSENS_CTRLA_SWRST;
    while (TSENS->SYNCBUSY.reg & TSENS_SYNCBUSY_SWRST);

    // 3. Load calibration (before ENABLE=1, enable-protected)
    tsens_load_calibration();

    // 4. Enable interrupt
    TSENS->INTENSET.reg = TSENS_INTENSET_RESRDY;
    NVIC_EnableIRQ(TSENS_IRQn);

    // 5. Enable
    TSENS->CTRLA.reg |= TSENS_CTRLA_ENABLE;
    while (TSENS->SYNCBUSY.reg & TSENS_SYNCBUSY_ENABLE);
}

void tsens_start(void) {
    TSENS->CTRLB.reg = TSENS_CTRLB_START;  // one-shot; no SYNCBUSY needed
}

void TSENS_Handler(void) {
    if (TSENS->INTFLAG.reg & TSENS_INTFLAG_RESRDY) {
        // Reading VALUE also clears RESRDY flag
        int32_t raw = (int32_t)((TSENS->VALUE.reg & 0xFFFFFF) << 8) >> 8; // sign-extend 24-bit
        // temperature_centidegC = raw; (100 counts = 1°C, e.g. 2500 = 25.00°C)
    }
    if (TSENS->INTFLAG.reg & TSENS_INTFLAG_OVF) {
        TSENS->INTFLAG.reg = TSENS_INTFLAG_OVF;
        // handle overflow — result is invalid
    }
}
```

## Free-Running Mode

```cpp
// Enable continuous measurements; RESRDY fires after each
// (must configure CTRLC before ENABLE=1 — enable-protected)

// In tsens_init(), before ENABLE:
TSENS->CTRLC.reg = TSENS_CTRLC_FREERUN;  // enable-protected

// After ENABLE, trigger first measurement:
TSENS->CTRLB.reg = TSENS_CTRLB_START;
// Subsequent measurements start automatically.
```

## Temperature Conversion

```cpp
// Raw VALUE is signed 24-bit in two's complement: 100 counts = 1°C
// Sign-extend 24-bit to 32-bit before arithmetic:
int32_t tsens_read_centidegC(void) {
    uint32_t raw24 = TSENS->VALUE.reg & 0xFFFFFF;
    int32_t val = (int32_t)(raw24 << 8) >> 8;  // arithmetic right shift sign-extends
    return val;  // divide by 100 for degrees C, or use directly as 0.01°C units
}

// Recommended: average 10 readings for accuracy:
int32_t tsens_average(void) {
    int64_t acc = 0;
    for (int i = 0; i < 10; i++) {
        TSENS->CTRLB.reg = TSENS_CTRLB_START;
        while (!(TSENS->INTFLAG.reg & TSENS_INTFLAG_RESRDY));
        uint32_t raw24 = TSENS->VALUE.reg & 0xFFFFFF;  // clears RESRDY
        acc += (int32_t)(raw24 << 8) >> 8;
    }
    return (int32_t)(acc / 10);
}
```

## Window Monitor (Interrupt on Temperature Threshold)

```cpp
// Fire WINMON interrupt when temperature > 85°C (8500 counts)
// Configure before ENABLE=1 (enable-protected registers)

// 85°C = 8500 counts; -40°C = -4000 counts
TSENS->CTRLC.reg = TSENS_CTRLC_WINMODE(0x1);  // ABOVE: VALUE > WINLT
TSENS->WINLT.reg = 8500;    // 85°C lower threshold (24-bit signed)
TSENS->INTENSET.reg = TSENS_INTENSET_WINMON;
```

## Key Facts

- Result: 24-bit signed, two's complement. 25°C = 2500 (0x09C4). −25°C = −2500.
- **Calibration required**: load GAIN, OFFSET, CAL from NVM before ENABLE=1.
- GAIN and OFFSET survive software reset — only power-on reset clears them.
- GAIN calibrated for GCLK_TSENS = OSC48M (48MHz). Scale GAIN if using other frequency.
- Enable-protected: CTRLA.RUNSTDBY, CTRLC, EVCTRL, WINLT, WINUT, GAIN, OFFSET, CAL.
- Write-synchronized: CTRLA.SWRST and CTRLA.ENABLE only.
- CTRLB.START: write-only, no SYNCBUSY required.
- INTFLAG.RESRDY and WINMON cleared by writing 1 OR by reading VALUE.
- STATUS.OVF=1: result required more than 24 bits — discard reading.
- Datasheet recommends averaging 10 measurements for best accuracy.

## See Also

- [[SAMC21 Datasheet Ch.43 TSENS]] — full register reference
- [[NVMCTRL]] — NVM Temperature Calibration Area location and structure
- [[Clock System]] — GCLK_TSENS; use OSC48M for factory calibration accuracy
- [[EVSYS]] — event-triggered measurements
- [[DMA Controller]] — DMA via RESRDY trigger
