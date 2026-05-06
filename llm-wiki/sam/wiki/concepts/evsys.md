---
title: EVSYS
type: concept
tags: [evsys, events, dma, timer, firmware, samc21]
sources: [samc21-datasheet-ch29-evsys]
created: 2026-05-05
updated: 2026-05-05
---

# EVSYS

Event System on the SAMC21. Routes signals between peripherals (timers, ADC,
DMA, EIC, etc.) without CPU involvement. 12 channels, asynchronous or
synchronous paths.

## Typical Init — TC Overflow Triggers ADC Start

```cpp
// Goal: TC0 overflow event → ADC0 start conversion, no CPU, no interrupt

// 1. APB clock for EVSYS
MCLK->APBCMASK.reg |= MCLK_APBCMASK_EVSYS;

// 2. GCLK for channel 0 (needed for sync/resync path)
GCLK->PCHCTRL[EVSYS_GCLK_ID_0].reg = GCLK_PCHCTRL_GEN_GCLK0
                                     | GCLK_PCHCTRL_CHEN;
while (!(GCLK->PCHCTRL[EVSYS_GCLK_ID_0].reg & GCLK_PCHCTRL_CHEN));

// 3. Configure generator peripheral: TC0 generates overflow event
TC0->COUNT16.EVCTRL.reg |= TC_EVCTRL_OVFEO;

// 4. Connect USER: ADC0 START (m=28) → channel 0
//    USERm.CHANNEL = channel_number + 1
EVSYS->USER[28].reg = EVSYS_USER_CHANNEL(0 + 1);

// 5. Configure channel 0: TC0 OVF generator, synchronous, rising edge
EVSYS->CHANNEL[0].reg = EVSYS_CHANNEL_EVGEN(0x34)   // TC0 OVF
                       | EVSYS_CHANNEL_PATH_SYNCHRONOUS
                       | EVSYS_CHANNEL_EDGSEL_RISING_EDGE;

// 6. Configure user peripheral: ADC0 starts on event
ADC0->EVCTRL.reg |= ADC_EVCTRL_STARTEI;
```

## Typical Init — Asynchronous Path (no GCLK)

```cpp
// EIC EXTINT5 → DMAC CH1 trigger, no GCLK needed

// USER first (m=6 for DMAC CH1), then channel
EVSYS->USER[6].reg = EVSYS_USER_CHANNEL(1 + 1);  // channel 1

// Async path: no EDGSEL, no GCLK required
EVSYS->CHANNEL[1].reg = EVSYS_CHANNEL_EVGEN(0x13)    // EIC EXTINT5
                       | EVSYS_CHANNEL_PATH_ASYNCHRONOUS
                       | EVSYS_CHANNEL_EDGSEL_NO_EVT_OUTPUT;
// No GCLK configuration needed for async path
```

## Software Event

```cpp
// Trigger channel 3 from software
EVSYS->SWEVT.reg = EVSYS_SWEVT_CHANNEL(1 << 3);
// Reads back as 0 — write-only strobe
```

## Overrun Detection

```cpp
// Check and clear overrun on channel 0 (sync/resync paths only)
if (EVSYS->INTFLAG.reg & EVSYS_INTFLAG_OVR(1 << 0)) {
    EVSYS->INTFLAG.reg = EVSYS_INTFLAG_OVR(1 << 0);  // w1c
    // handle overrun
}
```

## PATH[1:0] Selection Guide

| PATH | Use when | Latency |
|------|---------|---------|
| ASYNC (0x2) | Generator has no clock or different clock domain; user handles edge detection; no interrupt needed | ~0 |
| SYNC (0x0) | Generator and channel share the same GCLK | 1 GCLK cycle |
| RESYNC (0x1) | Generator and channel have different clocks | 3 GCLK cycles |

EDGSEL must be 0x0 (NO_EVT_OUTPUT) when PATH = ASYNC.

## EDGSEL Values (sync/resync only)

| Value | Name | Trigger |
|-------|------|---------|
| 0x0 | NO_EVT_OUTPUT | (no event output; required for async) |
| 0x1 | RISING_EDGE | Rising edge of generator signal |
| 0x2 | FALLING_EDGE | Falling edge |
| 0x3 | BOTH_EDGES | Both edges |

## Standby Operation

```cpp
// Keep channel 0 active through STANDBY for RTC-to-ADC event
EVSYS->CHANNEL[0].reg = EVSYS_CHANNEL_EVGEN(0x03)  // RTC ALARM0
                       | EVSYS_CHANNEL_PATH_ASYNCHRONOUS
                       | EVSYS_CHANNEL_EDGSEL_NO_EVT_OUTPUT
                       | EVSYS_CHANNEL_RUNSTDBY;
```

## Key USERm Indices

| m | Peripheral | Action |
|---|-----------|--------|
| 1–4 | PORT EV0–EV3 | External event input (async only) |
| 5–8 | DMAC CH0–CH3 | DMA trigger |
| 23 | TC0 | Timer event action |
| 24 | TC1 | Timer event action |
| 25 | TC2 | Timer event action |
| 28 | ADC0 START | Start ADC conversion |
| 29 | ADC0 SYNC | Flush ADC |
| 32 | SDADC START | Start SDADC (async only) |
| 34–37 | AC COMP0–COMP3 | Comparator start (async only) |
| 38 | DAC START | Start DAC (async only) |

## Key EVGEN Values

| EVGEN | Source |
|-------|--------|
| 0x01 | OSCCTRL FAIL (clock failure detection) |
| 0x03 | RTC CMP0 / ALARM0 |
| 0x06–0x0D | RTC PER0–PER7 (periodic) |
| 0x0E–0x1D | EIC EXTINT0–EXTINT15 |
| 0x1F–0x22 | DMAC CH0–CH3 |
| 0x34–0x36 | TC0 OVF/MC0/MC1 |
| 0x43 | ADC0 RESRDY |
| 0x49–0x4C | AC COMP0–COMP3 |
| 0x4F | DAC EMPTY |

## Key Facts

- USERm.CHANNEL = channel_number + 1 (0 = disconnected, 1 = ch0, 12 = ch11).
- Always write USER before configuring CHANNELn.
- CHANNELn must be written as a single 32-bit word.
- CHSTATUS.USRRDYn resets to 0xFF — do not rely on it as a "connected" indicator.
- Asynchronous path generates no INTFLAG.EVDn or OVRn flags — use for fire-and-forget.
- EVSYS is always enabled; use CTRLA.SWRST = 1 to reset all channels.
- CHANNELn resets to 0x00008000 (ONDEMAND=1) — GCLK requested on demand by default.

## See Also

- [[SAMC21 Datasheet Ch.29 EVSYS]] — full generator/user tables, register reference
- [[Clock System]] — GCLK_EVSYS_CHANNEL_n configuration via PCHCTRL
- [[DMA Controller]] — DMAC as event user (CH0–CH3) and generator
- [[TC 32-Bit Paired Mode]] — TC event actions (EVACT) and output enables (OVFEO/MCEO)
- [[RTC]] — RTC periodic and alarm event outputs
- [[EIC]] — EXTINT as event generators
- [[ADC]] — ADC START/SYNC event users
