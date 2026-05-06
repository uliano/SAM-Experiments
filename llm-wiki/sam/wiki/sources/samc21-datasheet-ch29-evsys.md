---
title: SAMC21 Datasheet Chapter 29 — EVSYS
type: source
tags: [evsys, events, dma, timer, samc21, datasheet]
date: 2026-05-05
---

# SAMC21 Datasheet Ch. 29 — EVSYS – Event System

Source: SAM C20/C21 Data Sheet DS60001479D, chapter 29, pages 463–483.

## Overview

The Event System (EVSYS) routes signals between peripherals without CPU
intervention. 12 configurable channels connect 95 event generators to 47 event
users. Three propagation paths are supported: asynchronous (zero latency, no
GCLK), synchronous (1 GCLK cycle), and resynchronized (3 GCLK cycles, handles
cross-domain events). The EVSYS is always enabled; only a software reset
(CTRLA.SWRST) is needed to reinitialize.

## Clocks

- **CLK_EVSYS_APB** (APBC): `MCLK->APBCMASK.reg |= MCLK_APBCMASK_EVSYS`
- **GCLK_EVSYS_CHANNEL_n**: dedicated GCLK per channel — required for sync and
  resync paths only; asynchronous path needs no GCLK.
- In standby: `CHANNELn.RUNSTDBY=1` keeps channel active; `CHANNELn.ONDEMAND=1`
  starts the channel GCLK on demand when an event arrives.

## Path Types

| PATH | Name | GCLK needed | Latency | Interrupts |
|------|------|-------------|---------|------------|
| 0x0 | SYNCHRONOUS | Yes (same as generator) | 1 GCLK | Yes |
| 0x1 | RESYNCHRONIZED | Yes (any) | 3 GCLK | Yes |
| 0x2 | ASYNCHRONOUS | No | ~0 | No |

- Asynchronous path: CHSTATUS/INTFLAG always 0; EDGSEL must be 0 (edge detection done by user peripheral).
- Overrun (OVRn) and EVD interrupts available only for sync/resync paths.

## Initialization Sequence

```
1. Enable event output in generator peripheral (e.g., TC.EVCTRL.OVFEO = 1)
2. EVSYS: write USERm.CHANNEL = channel_number + 1  ← critical offset!
3. EVSYS: configure CHANNELn (EVGEN, PATH, EDGSEL, RUNSTDBY)
4. Configure event action in user peripheral (e.g., TC.EVCTRL.EVACT)
5. Enable event input in user peripheral (e.g., TC.EVCTRL.STARTEI = 1)
```

**USERm.CHANNEL is 1-based**: 0 = no channel, 1 = channel 0, 2 = channel 1, ..., 12 = channel 11.

## Register Map

| Offset | Name | Reset | Description |
|--------|------|-------|-------------|
| 0x00 | CTRLA | 0x00 | SWRST(0) only |
| 0x0C | CHSTATUS | 0x000000FF | CHBUSYn[27:16] / USRRDYn[11:0] — read-only |
| 0x10 | INTENCLR | 0 | EVDn[27:16] / OVRn[11:0] |
| 0x14 | INTENSET | 0 | EVDn[27:16] / OVRn[11:0] |
| 0x18 | INTFLAG | 0 | EVDn[27:16] / OVRn[11:0] — w1c; 0 when async |
| 0x1C | SWEVT | 0 | CHANNELn[11:0] — write 1 fires software event; reads 0 |
| 0x20+n*4 | CHANNELn | 0x00008000 | Per-channel config (n=0..11) |
| 0x80+m*4 | USERm | 0 | Event user channel selection (m=0..49) |

## CHANNELn Register (32-bit, PAC protected)

| Field | Bits | Description |
|-------|------|-------------|
| EVGEN[7:0] | 7:0 | Event generator selection (0=none, see Table 29-2) |
| PATH[1:0] | 9:8 | 0=SYNC, 1=RESYNC, 2=ASYNC |
| EDGSEL[1:0] | 11:10 | Edge detection: 0=none, 1=RISING, 2=FALLING, 3=BOTH; must be 0 for async |
| RUNSTDBY | 14 | 1 = channel runs in STANDBY |
| ONDEMAND | 15 | 1 = GCLK requested on demand (reset default) |

Reset = 0x00008000 (ONDEMAND=1 by default).
**Must write all fields in a single 32-bit write** (not separate byte/halfword writes).

## Sleep Mode Behavior

| ONDEMAND | RUNSTDBY | Behavior |
|----------|----------|---------|
| 0 | 0 | Active in IDLE if event pending; disabled in STANDBY |
| 0 | 1 | Always active in IDLE and STANDBY |
| 1 | 0 | Active in IDLE on demand; disabled in STANDBY; +2 GCLK latency in RESYNC |
| 1 | 1 | Always active; runs in STANDBY; +2 GCLK latency in RESYNC |

## CHSTATUS (read-only)

| Field | Bits | Description |
|-------|------|-------------|
| USRRDYn[11:0] | 11:0 | Set = all users on channel n ready (reset = 0xFF, i.e. channels 0–7 "ready" by default) |
| CHBUSYn[11:0] | 27:16 | Set = event on channel n not yet handled by all users |

## Event Generators (Table 29-2, CHANNELn.EVGEN)

Selected key entries:

| EVGEN | Generator |
|-------|-----------|
| 0x00 | NONE |
| 0x01 | OSCCTRL FAIL |
| 0x02 | OSC32KCTRL FAIL |
| 0x03 | RTC CMP0 / ALARM0 |
| 0x05 | RTC OVF |
| 0x06–0x0D | RTC PER0–PER7 |
| 0x0E–0x1D | EIC EXTINT0–EXTINT15 |
| 0x1F–0x22 | DMAC CH0–CH3 |
| 0x23–0x29 | TCC0 (OVF/TRG/CNT/MC0–MC3) |
| 0x34–0x36 | TC0 OVF/MC0/MC1 |
| 0x37–0x39 | TC1 OVF/MC0/MC1 |
| 0x3A–0x3C | TC2 OVF/MC0/MC1 |
| 0x40–0x42 | TC4 OVF/MC0/MC1 |
| 0x43 | ADC0 RESRDY |
| 0x44 | ADC0 WINMON |
| 0x45 | ADC1 RESRDY |
| 0x47 | SDADC RESRDY |
| 0x49–0x4C | AC COMP0–COMP3 |
| 0x4F | DAC EMPTY |
| 0x56 | PAC ACCERR |
| 0x58–0x60 | TC5–TC7 (OVF/MC0/MC1) |

## Event Users (Table 29-3, USERm)

Selected key entries:

| m | User | Path |
|---|------|------|
| 0 | TSENS START | Async/Sync/Resync |
| 1–4 | PORT EV0–EV3 | Async only |
| 5–8 | DMAC CH0–CH3 | Async/Sync/Resync |
| 9–14 | TCC0 EV0/EV1/MC0–MC3 | Async/Sync/Resync |
| 15–18 | TCC1 EV0/EV1/MC0/MC1 | Async/Sync/Resync |
| 19–22 | TCC2 EV0/EV1/MC0/MC1 | Async/Sync/Resync |
| 23–27 | TC0–TC4 | Async/Sync/Resync |
| 28–29 | ADC0 START/SYNC | Async/Sync/Resync |
| 30–31 | ADC1 START/SYNC | Async/Sync/Resync |
| 32 | SDADC START | Async only |
| 33 | SDADC FLUSH | Async only |
| 34–37 | AC COMP0–COMP3 | Async only |
| 38 | DAC START | Async only |
| 47–49 | TC5–TC7 | Async/Sync/Resync |

## Key Facts

- EVSYS is always enabled; CTRLA.SWRST is the only control bit.
- USERm.CHANNEL = channel_number + 1 (offset by 1; 0 = disconnected).
- Always configure USER before CHANNELn.
- CHANNELn must be written as a single 32-bit word.
- Asynchronous PATH: no GCLK, no EDGSEL, no interrupts. Edge detection must be configured in the user peripheral.
- CHSTATUS.USRRDYn resets to 0xFF (channels 0–7 appear ready even without users connected).
- Overrun condition: new event before previous was handled by all users. Only for sync/resync.
- SWEVT allows CPU to trigger any channel via software.

## See Also

- [[EVSYS]] — firmware configuration patterns
- [[Clock System]] — GCLK_EVSYS_CHANNEL_n configuration
- [[DMA Controller]] — DMAC as event user and generator
- [[TC 32-Bit Paired Mode]] — TC as event user (EVACT) and generator (OVFEO/MCEO)
- [[RTC]] — RTC periodic and alarm events as generators
- [[EIC]] — EXTINT as event generators
