---
title: TCC Configuration
type: concept
tags: [tcc, timer, pwm, capture, firmware, samc21]
sources: [samc21-datasheet-ch36-tcc, samc21-errata]
created: 2026-05-05
updated: 2026-05-18
---

# TCC Configuration

TCC (Timer/Counter for Control Applications) on the SAMC21: advanced PWM with
dead-time insertion, pattern generation, and recoverable fault handling.
Three instances — TCC0/TCC1 share a GCLK channel, TCC2 is independent.

## Single-Slope PWM — TCC0, 20 kHz, 48 MHz

```cpp
// TCC0 WO[0] = PA04 (function E), single-slope PWM, 20 kHz, 50% duty
// PER+1 = 48MHz / (1 * 20kHz) = 2400 → PER = 2399

void tcc0_pwm_init(void) {
    // 1. Clocks
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_TCC0;
    GCLK->PCHCTRL[TCC0_GCLK_ID].reg =
        GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[TCC0_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));

    // 2. Reset
    TCC0->CTRLA.reg = TCC_CTRLA_SWRST;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_SWRST);

    // 3. Configure (enable-protected)
    TCC0->CTRLA.reg =
        TCC_CTRLA_PRESCALER_DIV1;   // no prescaler, 48 MHz

    TCC0->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_WAVE);

    // 4. Period and duty cycle
    TCC0->PER.reg = 2399;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_PER);

    TCC0->CC[0].reg = 1199;         // 50% duty
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_CC0);

    // 5. Enable
    TCC0->CTRLA.reg |= TCC_CTRLA_ENABLE;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_ENABLE);

    // 6. Configure pin PA04 as TCC0/WO[0] (function E)
    PORT->Group[0].PMUX[2].bit.PMUXE = MUX_PA04E_TCC0_WO0;
    PORT->Group[0].PINCFG[4].reg = PORT_PINCFG_PMUXEN;
}
```

## Update Duty Cycle (Double Buffer)

```cpp
void tcc0_set_duty(uint32_t cc0_value) {
    // Write to shadow buffer — copied to CC0 at next UPDATE (overflow)
    TCC0->CCBUF[0].reg = cc0_value;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_CC0);
}
```

## Dead-Time Insertion (H-Bridge, OTMX=0x1)

```cpp
// TCC0 with DTI: WO[0]/WO[4] = channel 0 low/high side
// Dead time = 100 cycles @ 48 MHz = ~2 µs

void tcc0_deadtime_init(void) {
    // ... (clock + reset as above) ...

    TCC0->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV1;

    TCC0->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_WAVE);

    TCC0->PER.reg = 2399;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_PER);
    TCC0->CC[0].reg = 1199;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_CC0);

    // Output matrix: pair LS/HS channels (OTMX=0x1)
    // Dead time: DTLS=100 (rising edge), DTHS=100 (falling edge)
    TCC0->WEXCTRL.reg =
        TCC_WEXCTRL_OTMX(0x1)
        | TCC_WEXCTRL_DTIEN0           // enable DTI on channel 0
        | TCC_WEXCTRL_DTLS(100)
        | TCC_WEXCTRL_DTHS(100);

    TCC0->CTRLA.reg |= TCC_CTRLA_ENABLE;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_ENABLE);
}
```

## BLDC Pattern Generation

```cpp
// Commutation sequence using PATT (override WO[0:5] for 3-phase bridge)
// Pattern bits: PGE[5:0]=1 (enable override), PGV = phase state

static const uint16_t commutation_table[6] = {
    // PGV[5:0] for each of 6 commutation steps
    // e.g., [PGE7..PGE0 | PGV7..PGV0] packed into PATT
    0x003F | (0b100001 << 16),   // step 0: U+/W-
    0x003F | (0b100100 << 16),   // step 1: U+/V-
    0x003F | (0b001100 << 16),   // step 2: V+/U- (example values)
    0x003F | (0b011000 << 16),
    0x003F | (0b010010 << 16),
    0x003F | (0b000011 << 16),
};

void tcc_set_commutation(uint8_t step) {
    // Write to PATTBUF — synchronized to UPDATE
    TCC0->PATTBUF.reg = commutation_table[step & 0x7];
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_PATT);
}
```

## Capture — Period and Pulse Width (PPW)

```cpp
// TCC1 captures period on CC0, pulse width on CC1 via EV1 (PPW mode)
// Input signal connected to TCC1 event input 1 (EV1)

void tcc1_capture_ppw_init(void) {
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_TCC1;
    GCLK->PCHCTRL[TCC1_GCLK_ID].reg =
        GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[TCC1_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));

    TCC1->CTRLA.reg = TCC_CTRLA_SWRST;
    while (TCC1->SYNCBUSY.reg & TCC_SYNCBUSY_SWRST);

    // Enable capture on CC0 and CC1
    TCC1->CTRLA.reg =
        TCC_CTRLA_PRESCALER_DIV16
        | TCC_CTRLA_CPTEN0
        | TCC_CTRLA_CPTEN1;

    // EV1 input → PPW (period+pulse width capture)
    TCC1->EVCTRL.reg =
        TCC_EVCTRL_TCEI1            // enable event input 1
        | TCC_EVCTRL_EVACT1(TCC_EVCTRL_EVACT1_PPW_Val);  // PPW action

    // Enable MC0/MC1 interrupts
    TCC1->INTENSET.reg = TCC_INTENSET_MC0 | TCC_INTENSET_MC1;
    NVIC_EnableIRQ(TCC1_IRQn);

    TCC1->CTRLA.reg |= TCC_CTRLA_ENABLE;
    while (TCC1->SYNCBUSY.reg & TCC_SYNCBUSY_ENABLE);
}

void TCC1_Handler(void) {
    uint32_t flags = TCC1->INTFLAG.reg;

    if (flags & TCC_INTFLAG_MC1) {
        TCC1->INTFLAG.reg = TCC_INTFLAG_MC1;
        uint32_t period = TCC1->CC[0].reg;     // full period
        uint32_t pulse  = TCC1->CC[1].reg;     // pulse width
        // process...
    }
}
```

## Count Register Read (READSYNC)

```cpp
uint32_t tcc_read_count(Tcc *tcc) {
    // Demand synchronization: request then poll
    tcc->CTRLBSET.reg = TCC_CTRLBSET_CMD_READSYNC;
    while (tcc->SYNCBUSY.reg & TCC_SYNCBUSY_COUNT);
    return tcc->COUNT.reg;
}
```

## One-Shot Mode

```cpp
// Single pulse then stop
TCC0->CTRLBSET.reg = TCC_CTRLBSET_ONESHOT;
while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_CTRLB);
// Retrigger by writing CMD=RETRIGGER
TCC0->CTRLBSET.reg = TCC_CTRLBSET_CMD_RETRIGGER;
while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_CTRLB);
```

## PWM Frequency Reference

| f_GCLK | Prescaler | PER | f_PWM | Resolution |
|--------|-----------|-----|-------|-----------|
| 48 MHz | DIV1 | 2399 | 20 kHz | ~11.2 bit |
| 48 MHz | DIV1 | 4799 | 10 kHz | ~12.2 bit |
| 48 MHz | DIV1 | 959 | 50 kHz | ~9.9 bit |
| 48 MHz | DIV8 | 2999 | 2 kHz | ~11.5 bit |
| 48 MHz | DIV1 | 47999 | 1 kHz | ~15.5 bit |

Dual-slope: halve the frequency for the same PER value.

## Key Facts

- Three instances: TCC0/TCC1 share GCLK channel; TCC2 is independent.
- Counter widths: TCC0/1 = 24-bit, TCC2 = 16-bit.
- WAVE, PER, CCx, PATT are write-synchronized — always poll SYNCBUSY after writes.
- COUNT read requires explicit READSYNC command via CTRLBSET.
- Double-buffered registers: CCBUFx, PERBUF, WAVEBUF, PATTBUF → shadow copies,
  applied at the UPDATE condition (overflow/underflow/re-trigger).
- CTRLB.LUPD=1 suspends buffer updates (useful for atomic multi-channel updates).
- CTRLA.ALOCK: auto-sets LUPD on each update event (useful for circular buffer DMA).
- WEXCTRL and DRVCTRL are enable-protected — configure before ENABLE=1.
- Current errata 1.21.9: when TCC uses EVSYS, route the EVSYS channel as
  `PATH_ASYNCHRONOUS`; TCC is not compatible with SYNC/RESYNC EVSYS mode on the
  target Rev F silicon.
- Fault filtering: async input filtered by FILTERVAL digital filter; BLANK window
  suppresses fault re-arm after each PWM edge to avoid glitch-triggered faults.
- Recoverable faults set STATUS.FAULTx; software must clear it for software halt mode.
- DBGCTRL.DBGRUN=1: TCC continues running when CPU halted by debugger.

## See Also

- [[SAMC21 Datasheet Ch.36 TCC]] — full register reference
- [[TC 32-Bit Paired Mode]] — simpler TC timer
- [[Clock System]] — GCLK_TCC setup, PCHCTRL
- [[NVIC Interrupt Map]] — TCC0/1/2 IRQn values
- [[I/O Multiplexing]] — TCC WO pin assignments
- [[EVSYS]] — fault input/capture event routing
