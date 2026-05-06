---
title: DAC Configuration
type: concept
tags: [dac, analog, firmware, samc21]
sources: [samc21-datasheet-ch41-dac]
created: 2026-05-05
updated: 2026-05-05
---

# DAC Configuration

DAC on the SAM C21: single channel, 10-bit, up to 350 ksps. SAM C21 only.
External output (VOUT pin) or internal routing to AC/ADC/SDADC. Two-stage FIFO
(DATABUF → DATA) enables DMA-driven waveform generation.

## Basic Init — INTREF, External Output

```cpp
// DAC output on VOUT pin, INTREF reference (bandgap, set via SUPC.VREF)
// Right-adjusted, 10-bit mode

void dac_init(void) {
    // 1. Clocks
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_DAC;
    GCLK->PCHCTRL[DAC_GCLK_ID].reg =
        GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[DAC_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));

    // 2. Reset
    DAC->CTRLA.reg = DAC_CTRLA_SWRST;
    while (DAC->SYNCBUSY.reg & DAC_SYNCBUSY_SWRST);

    // 3. Enable-protected config (ENABLE=0)
    DAC->CTRLB.reg =
        DAC_CTRLB_REFSEL_INTREF   // internal bandgap reference
        | DAC_CTRLB_EOEN;         // enable VOUT output buffer

    // 4. Enable
    DAC->CTRLA.reg |= DAC_CTRLA_ENABLE;
    while (DAC->SYNCBUSY.reg & DAC_SYNCBUSY_ENABLE);

    // 5. Wait for startup
    while (!(DAC->STATUS.reg & DAC_STATUS_READY));
}

void dac_write(uint16_t value_10bit) {
    DAC->DATA.reg = value_10bit & 0x3FF;
    while (DAC->SYNCBUSY.reg & DAC_SYNCBUSY_DATA);
}
```

## Internal Output (to AC or ADC)

```cpp
// DAC output routed internally to AC comparator or ADC input
// IOEN=1 enables internal path; EOEN=1 also needed if used by ADC
DAC->CTRLB.reg =
    DAC_CTRLB_REFSEL_INTREF
    | DAC_CTRLB_IOEN      // internal output to AC/ADC/SDADC
    | DAC_CTRLB_EOEN;     // output buffer must be on for ADC use
```

## Event-Triggered Output with DMA

```cpp
// DMA feeds DATABUF; timer event triggers START to push DATABUF → DATA
// This generates a continuous waveform without CPU involvement.

void dac_dma_init(void) {
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_DAC;
    GCLK->PCHCTRL[DAC_GCLK_ID].reg =
        GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[DAC_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));

    DAC->CTRLA.reg = DAC_CTRLA_SWRST;
    while (DAC->SYNCBUSY.reg & DAC_SYNCBUSY_SWRST);

    DAC->CTRLB.reg =
        DAC_CTRLB_REFSEL_INTREF
        | DAC_CTRLB_EOEN;

    // Enable STARTEI event input and EMPTYEO event output
    DAC->EVCTRL.reg =
        DAC_EVCTRL_STARTEI    // START event loads DATABUF → DATA
        | DAC_EVCTRL_EMPTYEO; // empty event drives DMA request

    DAC->CTRLA.reg |= DAC_CTRLA_ENABLE;
    while (DAC->SYNCBUSY.reg & DAC_SYNCBUSY_ENABLE);
    while (!(DAC->STATUS.reg & DAC_STATUS_READY));

    // DMA: SRCADDR=waveform_buf, DSTADDR=&DAC->DATABUF, TRIGSRC=DAC_DMAC_ID,
    //      TRIGACT=BEAT, BTCNT=N (circular descriptor for continuous output)
}
```

## Dithering Mode (14-bit Effective Resolution)

```cpp
// Dithering: 16 sub-conversions per output step, averaging to 14-bit precision
// CTRLB.DITHER=1, CTRLB.LEFTADJ=0 → DATA[13:4]=10-bit value, DATA[3:0]=dither
// EVSYS must generate 16 STARTEI events per DATABUF write (e.g. timer at 16× rate)

DAC->CTRLB.reg =
    DAC_CTRLB_REFSEL_INTREF
    | DAC_CTRLB_EOEN
    | DAC_CTRLB_DITHER;    // 14-bit dithering mode

// Write 14-bit value right-adjusted in DATA[13:0]:
// DATA[13:4] = upper 10 bits; DATA[3:0] = 4 dither bits
DAC->DATA.reg = (dac14_value & 0x3FFF);
while (DAC->SYNCBUSY.reg & DAC_SYNCBUSY_DATA);
```

## EMPTY Interrupt (polling new data)

```cpp
void DAC_Handler(void) {
    if (DAC->INTFLAG.reg & DAC_INTFLAG_EMPTY) {
        // DATABUF empty — load next sample
        DAC->DATABUF.reg = next_sample();
        while (DAC->SYNCBUSY.reg & DAC_SYNCBUSY_DATABUF);
        // Writing DATABUF also clears INTFLAG.EMPTY
    }
    if (DAC->INTFLAG.reg & DAC_INTFLAG_UNDERRUN) {
        DAC->INTFLAG.reg = DAC_INTFLAG_UNDERRUN;
        // handle underrun (missed START event with empty DATABUF)
    }
}
```

## Key Facts

- SAM C21 only — not present on SAM C20.
- `V_OUT = DATA / 0x3FF × VREF` (10-bit, right-adjusted).
- Enable-protected: CTRLB, EVCTRL — configure before ENABLE=1.
- Write-synchronized: CTRLA.SWRST/ENABLE, DATA, DATABUF — poll SYNCBUSY.
- Wait for STATUS.READY=1 after enabling before writing DATA.
- CTRLB.EOEN=1 required for VOUT pin output.
- CTRLB.IOEN=1 for internal use by AC/ADC/SDADC; EOEN must also be 1 for ADC.
- CTRLB.VPD=1 disables voltage pump — only safe above 2.5V VDDANA.
- CTRLB.LEFTADJ=1: value in DATA[15:6] instead of DATA[9:0].
- Two-stage FIFO: write DATABUF → START event transfers to DATA → conversion.
- UNDERRUN: START event occurred while DATABUF was already empty.
- CTRLA.RUNSTDBY=1: output buffer holds last value in STANDBY.

## See Also

- [[SAMC21 Datasheet Ch.41 DAC]] — full register reference
- [[SUPC]] — VREF level for INTREF
- [[Clock System]] — GCLK_DAC setup
- [[EVSYS]] — periodic START event for waveform generation
- [[DMA Controller]] — DMA feed via EMPTY DMA request
- [[AC Configuration]] — use DAC as AC negative reference (MUXNEG=DAC)
- [[ADC Configuration]] — use DAC as ADC reference (MUXPOS=DAC)
