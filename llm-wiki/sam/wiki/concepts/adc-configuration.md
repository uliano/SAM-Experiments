---
title: ADC Configuration
type: concept
tags: [adc, analog, firmware, samc21]
sources: [samc21-datasheet-ch38-adc]
created: 2026-05-05
updated: 2026-05-05
---

# ADC Configuration

ADC0 and ADC1 on the SAMC21: 12-bit SAR converter with 1 MSPS maximum rate,
differential/single-ended input, averaging, oversampling, window monitor,
automatic sequence scan, and DMA/event support.

## ADC0 Init — Single-Shot, AIN0, Internal Reference (INTREF)

```cpp
// ADC0 single-shot, AIN0, 12-bit, INTREF reference (bandgap)
// GCLK0 = 48 MHz, prescaler DIV32 → CLK_ADC = 1.5 MHz

void adc0_init(void) {
    // 1. Clocks
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_ADC0;
    GCLK->PCHCTRL[ADC0_GCLK_ID].reg =
        GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[ADC0_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));

    // 2. Load calibration from NVM Software Calibration Area
    uint32_t calib = *((uint32_t *)0x00806020);
    ADC0->CALIB.reg =
        ADC_CALIB_BIASCOMP((calib >> 2) & 0x7)
        | ADC_CALIB_BIASREFBUF((calib >> 5) & 0x7);

    // 3. Reset
    ADC0->CTRLA.reg = ADC_CTRLA_SWRST;
    while (ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_SWRST);

    // 4. Enable-protected config (while ENABLE=0)
    ADC0->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV32;

    ADC0->REFCTRL.reg = ADC_REFCTRL_REFSEL_INTREF;  // bandgap
    // Ensure SUPC.VREF.VREFOE=1 and SUPC.VREF.SEL set before this

    // 5. Input (double-buffered but must be set before first conversion)
    ADC0->INPUTCTRL.reg =
        ADC_INPUTCTRL_MUXPOS_AIN0
        | ADC_INPUTCTRL_MUXNEG_GND;   // single-ended
    while (ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_INPUTCTRL);

    // 6. 12-bit result, right-adjusted, single-ended
    ADC0->CTRLC.reg =
        ADC_CTRLC_RESSEL_12BIT;
    while (ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_CTRLC);

    // 7. Enable
    ADC0->CTRLA.reg |= ADC_CTRLA_ENABLE;
    while (ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_ENABLE);
}

uint16_t adc0_read(void) {
    // Trigger single conversion
    ADC0->SWTRIG.reg = ADC_SWTRIG_START;
    while (ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_SWTRIG);
    // Wait for result
    while (!(ADC0->INTFLAG.reg & ADC_INTFLAG_RESRDY));
    ADC0->INTFLAG.reg = ADC_INTFLAG_RESRDY;
    return ADC0->RESULT.reg;
}
```

## Free-Running Mode with RESRDY Interrupt

```cpp
void adc0_freerun_init(uint8_t ainpos) {
    // ... (clock + calibration + reset as above) ...

    ADC0->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV32;
    ADC0->REFCTRL.reg = ADC_REFCTRL_REFSEL_INTREF;

    ADC0->INPUTCTRL.reg =
        ADC_INPUTCTRL_MUXPOS(ainpos)
        | ADC_INPUTCTRL_MUXNEG_GND;
    while (ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_INPUTCTRL);

    ADC0->CTRLC.reg =
        ADC_CTRLC_RESSEL_12BIT
        | ADC_CTRLC_FREERUN;          // continuous conversion
    while (ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_CTRLC);

    ADC0->INTENSET.reg = ADC_INTENSET_RESRDY;
    NVIC_EnableIRQ(ADC0_IRQn);

    ADC0->CTRLA.reg |= ADC_CTRLA_ENABLE;
    while (ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_ENABLE);

    // Start free-running
    ADC0->SWTRIG.reg = ADC_SWTRIG_START;
    while (ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_SWTRIG);
}

void ADC0_Handler(void) {
    ADC0->INTFLAG.reg = ADC_INTFLAG_RESRDY;
    uint16_t result = ADC0->RESULT.reg;
    // process result...
}
```

## Averaging — 16 Samples, 12-bit Output

```cpp
// Configure 16-sample averaging → result is averaged 12-bit value
// Sampling rate reduced by factor 16
ADC0->AVGCTRL.reg =
    ADC_AVGCTRL_SAMPLENUM_16   // 0x4 = 16 samples
    | ADC_AVGCTRL_ADJRES(4);   // right-shift by 4 to get 12-bit
while (ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_AVGCTRL);

// CTRLC.RESSEL must be set (not 8-bit) for averaging ≥ 2 samples:
ADC0->CTRLC.reg |= ADC_CTRLC_RESSEL_16BIT;  // accumulator register width
while (ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_CTRLC);
```

## Oversampling to 16-bit Resolution

```cpp
// 256 samples accumulated, auto right-shift 4, ADJRES=0 → 16-bit result in RESULT
ADC0->AVGCTRL.reg =
    ADC_AVGCTRL_SAMPLENUM_256  // 0x8
    | ADC_AVGCTRL_ADJRES(0);
while (ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_AVGCTRL);

ADC0->CTRLC.reg |= ADC_CTRLC_RESSEL_16BIT;
while (ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_CTRLC);
// RESULT[15:0] contains 16-bit oversampled value after each conversion sequence
```

## DMA Transfer of Results

```cpp
// DMA channel: TRIGSRC=ADC0_DMAC_ID (result ready), TRIGACT=BEAT
// SRCADDR = &ADC0->RESULT.reg (fixed)
// DSTADDR = result_buf + N (end address)
// BTCNT = N
// Reading RESULT register clears the RESRDY DMA request
```

## Automatic Sequence Scan (AIN0–AIN3)

```cpp
// Scan AIN0, AIN1, AIN2, AIN3 automatically on each trigger
ADC0->SEQCTRL.reg =
    (1u << 0)   // enable AIN0
    | (1u << 1) // enable AIN1
    | (1u << 2) // enable AIN2
    | (1u << 3);// enable AIN3
// Each trigger starts a sequence; RESRDY fires after each conversion.
// SEQSTATUS.SEQSTATE tracks which channel just completed.
```

## Window Monitor

```cpp
// Generate interrupt when AIN0 > 2048 (half of 4096 full-scale)
ADC0->WINLT.reg = 0;
while (ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_WINLT);
ADC0->WINUT.reg = 2048;
while (ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_WINUT);

ADC0->CTRLC.reg |= ADC_CTRLC_WINMODE(1);  // Result > WINLT
while (ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_CTRLC);

ADC0->INTENSET.reg = ADC_INTENSET_WINMON;
```

## Offset and Gain Correction

```cpp
// Enable hardware correction: result = (raw - OFFSETCORR) * GAINCORR
// GAINCORR is a 12-bit unsigned value; 1.0 = 0x800 (2048)
ADC0->OFFSETCORR.reg = -10;    // signed 12-bit, subtract 10 LSBs offset
while (ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_OFFSETCORR);

ADC0->GAINCORR.reg = 0x800;    // unity gain
while (ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_GAINCORR);

ADC0->CTRLC.reg |= ADC_CTRLC_CORREN;
while (ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_CTRLC);
```

## Calibration Loading from NVM

```cpp
// NVM Software Calibration Area starts at 0x00806020
// ADC0 BIASCOMP: bits [5:3], ADC0 BIASREFBUF: bits [2:0]
// ADC1 BIASCOMP: bits [11:9], ADC1 BIASREFBUF: bits [8:6]
void adc_load_calibration(void) {
    uint16_t cal = *((uint16_t *)0x00806020);

    ADC0->CALIB.reg =
        ADC_CALIB_BIASCOMP((cal >> 2) & 0x7)
        | ADC_CALIB_BIASREFBUF((cal >> 5) & 0x7);

    ADC1->CALIB.reg =
        ADC_CALIB_BIASCOMP((cal >> 10) & 0x7)
        | ADC_CALIB_BIASREFBUF((cal >> 13) & 0x7);
}
```

## Sampling Rate Reference

`f_CLK_ADC = f_GCLK_ADC / 2^(1 + PRESCALER)`  
Minimum conversion time (12-bit, SAMPLEN=0): `(1 + 12) / f_CLK_ADC`  
For 1 MSPS @ 12-bit: `f_CLK_ADC = (1 + 12 − 1) × 1 MSPS = 16 MHz`

| GCLK_ADC | PRESCALER | f_CLK_ADC | SAMPLEN | Rate |
|----------|-----------|-----------|---------|------|
| 48 MHz | DIV2 (0x0) | 24 MHz | 0 | ~1.85 MSPS (max rate) |
| 32 MHz | DIV2 (0x0) | 16 MHz | 0 | 1 MSPS |
| 48 MHz | DIV4 (0x1) | 12 MHz | 0 | ~923 kSPS |
| 48 MHz | DIV32 (0x4) | 1.5 MHz | 0 | ~115 kSPS |
| 48 MHz | DIV256 (0x7) | 187.5 kHz | 0 | ~14.4 kSPS |

## Key Facts

- Minimum prescaler is DIV2 — there is no divide-by-1 option.
- CTRLB, REFCTRL, EVCTRL, CALIB are enable-protected — configure before ENABLE=1.
- Double-buffered registers (INPUTCTRL, CTRLC, AVGCTRL, SAMPCTRL, WINLT, WINUT,
  GAINCORR, OFFSETCORR) apply to the ADC at the start of the next conversion.
  Always poll SYNCBUSY after writing them.
- Rail-to-rail mode (CTRLC.R2R=1): requires SAMPCTRL.OFFCOMP=1 (fixes sampling at 4 cycles).
- OVERRUN interrupt: result was not read before next conversion completed.
  Not a sleep wakeup source.
- Calibration must be loaded from NVM before ENABLE=1 for accurate results.
- CTRLC.RESSEL must be set (not 8-bit) when using accumulation (SAMPLENUM > 0).
- Master/slave: ADC1.CTRLA.SLAVEEN=1 → ADC1 slaves to ADC0; both can sample simultaneously.
- Event trigger: ADC uses async event channel; STARTINV=1 for falling-edge trigger.

## See Also

- [[SAMC21 Datasheet Ch.38 ADC]] — full register reference
- [[SUPC]] — internal reference voltage level (VREF.SEL)
- [[Clock System]] — GCLK_ADC clock setup
- [[NVMCTRL]] — NVM calibration area for BIASCOMP/BIASREFBUF
- [[DMA Controller]] — ADC result DMA transfer
- [[EVSYS]] — event-triggered conversions
