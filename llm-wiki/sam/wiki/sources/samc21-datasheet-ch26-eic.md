---
title: SAMC21 Datasheet Chapter 26 — EIC
type: source
tags: [eic, extint, interrupt, wakeup, samc21, datasheet]
date: 2026-05-05
---

# SAMC21 Datasheet Ch. 26 — EIC – External Interrupt Controller

Source: SAM C20/C21 Data Sheet DS60001479D, chapter 26, pages 385–409.

## Overview

The External Interrupt Controller (EIC) allows up to 16 external pins (EXTINT0–15)
and one NMI pin to generate interrupts or events on edge or level conditions.
Each channel can be filtered (majority-vote, 3 samples) and configured for
synchronous or asynchronous detection. Asynchronous mode enables wakeup from
STANDBY without a running GCLK.

Note: The SAMC21J18A is **not** an "N" variant — DEBOUNCEN, DPRESCALER, and
PINSTATE registers are not present.

## Base Address / Clocks

- APB bus: APBB (not APBA); enable: `MCLK->APBBMASK.reg |= MCLK_APBBMASK_EIC`
- CLK_EIC_APB: always needed for register access
- GCLK_EIC (CTRLA.CKSEL=0) or CLK_ULP32K (CTRLA.CKSEL=1): required only for
  synchronous edge detection or filtering. Level detection and async edge
  detection do NOT require GCLK.
- CTRLA.CKSEL is enable-protected; configure before ENABLE=1.

## Initialization Sequence

```cpp
// 1. APB clock
MCLK->APBBMASK.reg |= MCLK_APBBMASK_EIC;

// 2. Optional GCLK_EIC (if filtering or sync edge detection needed)
GCLK->PCHCTRL[EIC_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
while (!(GCLK->PCHCTRL[EIC_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));

// 3. Configure sense + filter for EIC channel 0 (enable-protected)
EIC->CONFIG[0].reg = EIC_CONFIG_SENSE0_RISE | EIC_CONFIG_FILTEN0;

// 4. Configure asynchronous mode for wakeup (enable-protected)
EIC->ASYNCH.reg |= EIC_ASYNCH_ASYNCH(1 << 0);  // channel 0 async

// 5. Enable interrupt
EIC->INTENSET.reg = EIC_INTENSET_EXTINT(1 << 0);
NVIC_EnableIRQ(EIC_EXTINT_0_IRQn);

// 6. Enable EIC
EIC->CTRLA.reg = EIC_CTRLA_ENABLE;
while (EIC->SYNCBUSY.reg & EIC_SYNCBUSY_ENABLE);
```

## SENSE Values (CONFIGn.SENSEx and NMISENSE)

| Value | Name | Condition |
|-------|------|-----------|
| 0x0 | NONE | No detection |
| 0x1 | RISE | Rising-edge detection |
| 0x2 | FALL | Falling-edge detection |
| 0x3 | BOTH | Both-edge detection |
| 0x4 | HIGH | High-level detection |
| 0x5 | LOW | Low-level detection |
| 0x6–0x7 | — | Reserved |

## Detection Latency

| Mode | Worst-case latency |
|------|--------------------|
| Level without filter | 5 CLK_EIC_APB periods |
| Level/edge without filter (sync) | 4 GCLK/ULP32K + 5 CLK_EIC_APB |
| Edge with filter | 6 GCLK/ULP32K + 5 CLK_EIC_APB |

## Register Summary

| Offset | Name | Description |
|--------|------|-------------|
| 0x00 | CTRLA | CKSEL(4)/ENABLE(1)/SWRST(0); CKSEL enable-protected; ENABLE/SWRST write-synchronized |
| 0x01 | NMICTRL | NMIASYNCH(4)/NMIFILTEN(3)/NMISENSE[2:0] |
| 0x02 | NMIFLAG | NMI(0) — write-1-to-clear |
| 0x04 | SYNCBUSY | ENABLE(1)/SWRST(0) |
| 0x08 | EVCTRL | EXTINTEO[15:0] — event output enable per channel (enable-protected) |
| 0x0C | INTENCLR | EXTINT[15:0] — interrupt enable clear |
| 0x10 | INTENSET | EXTINT[15:0] — interrupt enable set |
| 0x14 | INTFLAG | EXTINT[15:0] — write-1-to-clear |
| 0x18 | ASYNCH | ASYNCH[15:0] — async mode per channel (enable-protected) |
| 0x1C | CONFIG0 | FILTEN[7:0]/SENSE[7:0] for channels 0–7 (enable-protected) |
| 0x20 | CONFIG1 | FILTEN[15:8]/SENSE[15:8] for channels 8–15 (enable-protected) |

CONFIG layout per channel x (within CONFIGn):
- Bit `4x+3` = FILTENx (filter enable)
- Bits `4x+2:4x` = SENSEx[2:0]

## Enable-Protected Registers

EVCTRL, CONFIGn, ASYNCH, CTRLA.CKSEL — all must be written before CTRLA.ENABLE=1,
or the EIC must be disabled (ENABLE=0; wait SYNCBUSY.ENABLE=0) before modifying.

## Key Facts

- FILTENx and ASYNCH[x] are mutually exclusive: filter must be disabled when
  async detection is active for that channel.
- Async mode (ASYNCH[x]=1) allows wakeup from STANDBY without any GCLK running.
- Level detection is inherently asynchronous when filtering is disabled.
- Each EXTINTx has a dedicated IRQ line; NMI has its own line (non-maskable).
- INTFLAG is not PAC-protected; INTENSET/INTENCLR are PAC-protected.
- If one EXTINT is shared across multiple I/O pins, only the first-programmed
  pin will be active.
- EIC continues to function in debug halt if interrupts are not serviced.

## See Also

- [[EIC]] — firmware configuration patterns
- [[I/O Multiplexing]] — EXTINT pin assignments (function A)
- [[PORT]] — pin input buffer enable (PINCFG.INEN)
- [[PM]] — standby wakeup via EIC async mode
- [[NVIC Interrupt Map]] — EIC_EXTINT_0 through EIC_EXTINT_15 IRQn values
