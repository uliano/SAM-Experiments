---
title: TC 32-Bit Paired Mode
type: concept
tags: [tc, timer, counter, pwm, firmware, samc21]
sources: [application-bring-up, samc21-datasheet-ch35-tc]
created: 2026-05-05
updated: 2026-05-05
---

# TC 32-Bit Paired Mode

How to configure and use TC instances on the SAMC21 — covering all three modes
(8-bit, 16-bit, 32-bit paired) with register-level reference.

## Pairing for 32-bit Mode

Adjacent even+odd TC pairs share a 32-bit counter. Only the **even (master)** TC
is configured; the odd (slave) TC is controlled automatically.

| Master (even) | Slave (odd) |
|--------------|------------|
| TC0 | TC1 |
| TC2 | TC3 |
| TC4 | TC5 |

Both master and slave need APB clock enabled. Only the master needs a GCLK.
All COUNT32 registers are accessed through the master TC. STATUS.SLAVE is
set on the slave when its master is in COUNT32 mode.

## Clock Setup

```cpp
MCLK->APBCMASK.reg |= MCLK_APBCMASK_TC0 | MCLK_APBCMASK_TC1;
GCLK->PCHCTRL[TC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
while (!(GCLK->PCHCTRL[TC0_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));
```

## Initialization (32-bit Counter)

```cpp
// Reset master
tc->COUNT32.CTRLA.reg = TC_CTRLA_SWRST;
while (tc->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_SWRST);

// Configure CTRLA (enable-protected — write before enabling)
tc->COUNT32.CTRLA.reg =
    TC_CTRLA_MODE_COUNT32 |
    TC_CTRLA_PRESCALER_DIV1024;

// Enable
tc->COUNT32.CTRLA.reg |= TC_CTRLA_ENABLE;
while (tc->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_ENABLE);
```

MODE_COUNT32 in the master CTRLA automatically configures the slave.

## Reading the Count (READSYNC Required)

Direct read of COUNT.reg returns stale data. Issue READSYNC first:

```cpp
tc->COUNT32.CTRLBSET.reg = TC_CTRLBSET_CMD_READSYNC;
while (tc->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_COUNT);
uint32_t count = tc->COUNT32.COUNT.reg;
```

## Waveform Generation (NPWM Example, 16-bit)

```cpp
tc->COUNT16.CTRLA.reg =
    TC_CTRLA_MODE_COUNT16 |
    TC_CTRLA_PRESCALER_DIV1;
tc->COUNT16.WAVE.reg = TC_WAVE_WAVEGEN_NPWM;   // TOP = 0xFFFF
tc->COUNT16.CC[0].reg = period_ticks - 1;       // period
tc->COUNT16.CC[1].reg = duty_ticks;             // duty (for MPWM)
tc->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
while (tc->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_ENABLE);
```

## Waveform Modes (WAVE.WAVEGEN)

| Value | Name | TOP | Output on match | Output on wrap |
|-------|------|-----|----------------|---------------|
| 0x0 | NFRQ | PER/MAX | Toggle WO[0] | No action |
| 0x1 | MFRQ | CC0 | Toggle WO[0] | No action |
| 0x2 | NPWM | PER/MAX | Set | Clear |
| 0x3 | MPWM | CC0 | Set WO[0]; CC1→WO[1] | Clear |

8-bit mode: TOP=PER register. 16/32-bit: TOP=MAX (0xFFFF / 0xFFFFFFFF) for NFRQ/NPWM.

Frequency formula: `f = f_GCLK_TC / (prescaler × (TOP + 1))`

## Prescaler Table

| Value | Divisor | f at 48 MHz GCLK |
|-------|---------|-----------------|
| DIV1 | 1 | 48 MHz |
| DIV2 | 2 | 24 MHz |
| DIV4 | 4 | 12 MHz |
| DIV8 | 8 | 6 MHz |
| DIV16 | 16 | 3 MHz |
| DIV64 | 64 | 750 kHz |
| DIV256 | 256 | 187.5 kHz |
| DIV1024 | 1024 | 46,875 Hz |

## Register Reference

### CTRLA Key Bits (enable-protected)

| Bits | Name | Description |
|------|------|-------------|
| 17 | CAPTEN1 | Enable capture on CC1 (0=compare) |
| 16 | CAPTEN0 | Enable capture on CC0 |
| 11 | ALOCK | Auto-lock: LUPD set on overflow/retrigger |
| 10:8 | PRESCALER[2:0] | See prescaler table |
| 6 | RUNSTDBY | Keep running in standby |
| 5:4 | PRESCSYNC[1:0] | GCLK(0)/PRESC(1)/RESYNC(2) |
| 3:2 | MODE[1:0] | COUNT16(0)/COUNT8(1)/COUNT32(2) |
| 1 | ENABLE | Enable |
| 0 | SWRST | Software reset |

### CTRLB CMD Values (write to CTRLBSET)

| Value | Name | Action |
|-------|------|--------|
| 0x1 | RETRIGGER | Force start/restart |
| 0x2 | STOP | Force stop |
| 0x3 | UPDATE | Force buffer→CC transfer |
| 0x4 | READSYNC | Synchronize COUNT for read |

CTRLB also: ONESHOT(2) stop on overflow; LUPD(1) lock update; DIR(0) count down.

### INTFLAG / INTENCLR / INTENSET Bits

| Bit | Name | Condition |
|-----|------|-----------|
| 5 | MC1 | Compare match or capture ch1 |
| 4 | MC0 | Compare match or capture ch0 |
| 1 | ERR | Capture overflow (MCx still set when new capture arrives) |
| 0 | OVF | Overflow or underflow |

### STATUS Bits (reset=0x01)

| Bit | Name | Description |
|-----|------|-------------|
| 5 | CCBUFV1 | CC1 buffer valid |
| 4 | CCBUFV0 | CC0 buffer valid |
| 3 | PERBUFV | PER buffer valid (8-bit only) |
| 1 | SLAVE | Set when this TC is a slave in 32-bit pairing |
| 0 | STOP | Reset=1; cleared when counter starts |

### SYNCBUSY Bits (0x10)

| Bit | Name | Wait after |
|-----|------|-----------|
| 7 | CC1 | CC1 write |
| 6 | CC0 | CC0 write |
| 4 | COUNT | COUNT write or READSYNC |
| 2 | CTRLB | CTRLB write |
| 1 | ENABLE | ENABLE write |
| 0 | SWRST | SWRST write |

### Mode-Specific Register Offsets

| Register | 8-bit | 16-bit | 32-bit |
|---------|-------|--------|--------|
| COUNT | 0x14 | 0x14 | 0x14 |
| PER | 0x1B | — | — |
| CC0 | 0x1C | 0x1C | 0x1C |
| CC1 | 0x1D | 0x1E | 0x20 |
| PERBUF | 0x2F | — | — |
| CCBUF0 | 0x30 | 0x30 | 0x30 |
| CCBUF1 | 0x31 | 0x32 | 0x34 |

## See Also

- [[SAMC21 Datasheet Ch.35 TC]] — full register reference, capture modes, event system
- [[Clock System]] — GCLK setup and PCHCTRL for TC
- [[Memory Map]] — TC0–TC5 base addresses
- [[NVIC Interrupt Map]] — TC IRQn values
