---
title: SDADC Configuration
type: concept
tags: [sdadc, sigma-delta, analog, firmware, samc21]
sources: [samc21-datasheet-ch39-sdadc]
created: 2026-05-05
updated: 2026-05-05
---

# SDADC Configuration

SDADC (Sigma-Delta ADC) on the SAM C21: 16-bit signed differential converter with
3rd-order SINC decimation filter. SAM C21 only. Three differential channel pairs,
gain/offset/shift post-correction, chopper mode for offset reduction.

## Basic Init — Channel 0, OSR64, INTREF

```cpp
// SDADC channel 0 (AINP0/AINN0), OSR=64, INTREF reference
// GCLK0=48MHz → prescaler=11 → CLK_SDADC=48MHz/(2×12)=2MHz
// f_FS = 2MHz/4 = 500kHz
// f_out = 500kHz/64 = 7812.5 sps

void sdadc_init(void) {
    // 1. Clocks
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_SDADC;
    GCLK->PCHCTRL[SDADC_GCLK_ID].reg =
        GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[SDADC_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));

    // 2. Reset
    SDADC->CTRLA.reg = SDADC_CTRLA_SWRST;
    while (SDADC->SYNCBUSY.reg & SDADC_SYNCBUSY_SWRST);

    // 3. Enable-protected config (ENABLE=0)
    SDADC->REFCTRL.reg =
        SDADC_REFCTRL_REFSEL_INTREF
        | SDADC_REFCTRL_REFRANGE(0x1)   // 1.4V < Vref < 2.4V (for INTREF ~1.65V)
        | SDADC_REFCTRL_ONREFBUF;       // enable reference buffer

    SDADC->CTRLB.reg =
        SDADC_CTRLB_PRESCALER(11)       // DIV = 2*(11+1) = 24 → 48MHz/24 = 2MHz
        | SDADC_CTRLB_OSR(SDADC_CTRLB_OSR_OSR64_Val)
        | SDADC_CTRLB_SKPCNT(0);        // skip 0 extra samples (3 skipped internally)

    // 4. Input channel 0
    SDADC->INPUTCTRL.reg = SDADC_INPUTCTRL_MUXSEL(0x0);  // channel 0
    while (SDADC->SYNCBUSY.reg & SDADC_SYNCBUSY_INPUTCTRL);

    // 5. Enable RESRDY interrupt
    SDADC->INTENSET.reg = SDADC_INTENSET_RESRDY;
    NVIC_EnableIRQ(SDADC_IRQn);

    // 6. Enable
    SDADC->CTRLA.reg |= SDADC_CTRLA_ENABLE;
    while (SDADC->SYNCBUSY.reg & SDADC_SYNCBUSY_ENABLE);
}

// Trigger first conversion
void sdadc_start(void) {
    SDADC->SWTRIG.reg = SDADC_SWTRIG_START;
    while (SDADC->SYNCBUSY.reg & SDADC_SYNCBUSY_SWTRIG);
}

void SDADC_Handler(void) {
    if (SDADC->INTFLAG.reg & SDADC_INTFLAG_RESRDY) {
        int16_t result = (int16_t)(SDADC->RESULT.reg & 0xFFFF);
        SDADC->INTFLAG.reg = SDADC_INTFLAG_RESRDY;
        // process signed 16-bit differential result
    }
}
```

## Free-Running Mode

```cpp
// Enable continuous conversion (no software trigger needed after first start)
SDADC->CTRLC.reg |= SDADC_CTRLC_FREERUN;
while (SDADC->SYNCBUSY.reg & SDADC_SYNCBUSY_CTRLC);
// After enabling the SDADC, trigger once to start; subsequent conversions are automatic.
```

## Offset and Gain Correction

```cpp
// Correction formula: Result = (Data0 + OFFSETCORR) × GAINCORR / 2^SHIFTCORR
// GAINCORR unity = 0x2000 (16384); SHIFTCORR=0 means no shift.

void sdadc_set_correction(int32_t offset, uint16_t gain, uint8_t shift) {
    SDADC->OFFSETCORR.reg = (uint32_t)(offset & 0xFFFFFF);  // 24-bit signed
    while (SDADC->SYNCBUSY.reg & SDADC_SYNCBUSY_OFFSETCORR);

    SDADC->GAINCORR.reg = gain & 0x3FFF;  // 14-bit
    while (SDADC->SYNCBUSY.reg & SDADC_SYNCBUSY_GAINCORR);

    SDADC->SHIFTCORR.reg = shift & 0xF;
    while (SDADC->SYNCBUSY.reg & SDADC_SYNCBUSY_SHIFTCORR);
}
```

## Channel Sequence Scan (3 Channels)

```cpp
// Scan all 3 channels in sequence on each trigger
SDADC->SEQCTRL.reg =
    SDADC_SEQCTRL_SEQEN(0)   // enable channel 0
    | SDADC_SEQCTRL_SEQEN(1) // enable channel 1
    | SDADC_SEQCTRL_SEQEN(2);// enable channel 2
// Each start trigger scans all enabled channels.
// SEQSTATUS.SEQSTATE identifies last completed channel.
```

## OSR and Sample Rate Reference

`f_CLK_SDADC = f_GCLK / (2 × (PRESCALER + 1))`  
`f_output = f_CLK_SDADC / (4 × OSR)`

| f_GCLK | PRESCALER | f_CLK_SDADC | OSR | f_output |
|--------|-----------|-------------|-----|----------|
| 48 MHz | 11 | 2 MHz | 64 | 7812 sps |
| 48 MHz | 11 | 2 MHz | 128 | 3906 sps |
| 48 MHz | 5 | 4 MHz | 64 | 15625 sps |
| 48 MHz | 2 | 8 MHz | 64 | 31250 sps |
| 48 MHz | 0 | 24 MHz | 64 | ~93750 sps |

Note: CLK_SDADC must meet electrical specifications from datasheet.

## Key Facts

- SAM C21 only — not present on SAM C20.
- Result is signed 16-bit in RESULT[15:0]. Full range: −32768 to +32767.
- OSR must not be changed while the SDADC is running; reset it first.
- First valid result starts from 3rd sample onward (sigma-delta settling).
  Use SKPCNT to discard additional startup samples.
- Enable-protected: CTRLA.ONDEMAND/RUNSTDBY, CTRLB, CTRLC, EVCTRL, ANACTRL.
- REFCTRL is enable-protected; configure reference before ENABLE=1.
- ONREFBUF=1 required when using INTREF or DAC as reference.
- REFRANGE must match actual reference voltage for proper ADC operation.
- Correction formula: Result = (Data0 + OFFSETCORR) × GAINCORR / 2^SHIFTCORR;
  saturated to 16-bit unsigned, then signed interpretation.
- Chopper mode (ANACTRL.ONCHOP=1): reduces offset error at modulator input.
- Write-synchronized registers emit a bus error if written while SYNCBUSY is set.
- INTFLAG.RESRDY and INTFLAG.WINMON are also cleared by reading RESULT.
- SDADC uses only asynchronous event channels (same as ADC).

## See Also

- [[SAMC21 Datasheet Ch.39 SDADC]] — full register reference
- [[ADC Configuration]] — SAR ADC (higher speed, single-ended option)
- [[SUPC]] — VREF for INTREF voltage level
- [[Clock System]] — GCLK_SDADC setup
- [[EVSYS]] — event-triggered SDADC conversions
