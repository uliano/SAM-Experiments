---
title: SAMC21 Datasheet Ch.44 FREQM
type: source
tags: [freqm, frequency, clock, samc21, datasheet]
sources: [samc21-datasheet-ch44-freqm]
created: 2026-05-05
updated: 2026-05-05
---

# SAMC21 Datasheet Ch.44 FREQM

Frequency Meter on the SAM C20/C21. Measures the frequency of one clock by
counting its cycles over a known number of reference clock cycles.

## Abstract

The FREQM measures the frequency of a measurement clock (GCLK_FREQM_MSR) by
counting how many of its cycles occur during a programmable number of reference
clock periods (CFGA.REFNUM). The 24-bit result VALUE is read after the DONE
interrupt or STATUS.BUSY=0. Both SAM C20 and SAM C21 include one FREQM instance.
DIVREF (÷8 reference prescaler) is available only on N-series variants.

## Key Facts

- Formula: `f_CLK_MSR = (VALUE / REFNUM) × f_CLK_REF`
- **Reference must be slower than the measured clock** — datasheet requirement.
- VALUE is 24-bit unsigned in VALUE[23:0].
- REFNUM (CFGA[7:0]): 1–255 reference cycles; non-zero required.
- CFGA is enable-protected: write REFNUM before CTRLA.ENABLE=1.
- CTRLA.ENABLE and CTRLA.SWRST are write-synchronized (poll SYNCBUSY).
- CTRLB.START: write-only, starts one measurement; no SYNCBUSY needed.
- STATUS.OVF: sticky flag — counter overflowed, VALUE invalid. Clear by writing 1 to OVF.
  Fix: reduce REFNUM or use a slower reference clock.
- INTFLAG.DONE: cleared by writing 1. Set on STATUS.BUSY 1→0 transition.
- PAC write-protection: CTRLA, CFGA, INTENCLR, INTENSET (not CTRLB, INTFLAG, STATUS).
- APB base: 0x40002C00 (APB-A, enabled at reset — no MCLK mask needed).
- GCLK PCHCTRL[3] = GCLK_FREQM_MSR (measurement clock).
- GCLK PCHCTRL[4] = GCLK_FREQM_REF (reference clock).
- NVIC line 4. No DMA, no events.
- Sleep: continues operating in Idle sleep. Disable before STANDBY for lowest power.
- Debug mode: FREQM continues normally when CPU is halted.
- DIVREF (CFGA bit 15): divides reference by 8; **N-series only**.

## Principle of Operation

FREQM uses one timer running from GCLK_FREQM_REF (the reference) and one
counter running from GCLK_FREQM_MSR (the measured clock). The reference timer
counts REFNUM reference cycles; during this window, the counter increments on
every measured clock edge. The counter value at window end is stored in VALUE.

## Register Summary

| Offset | Name | Key Fields |
|--------|------|-----------|
| 0x00 | CTRLA | ENABLE (bit 1), SWRST (bit 0) — both write-synchronized, PAC-protected |
| 0x01 | CTRLB | START (bit 0) — write-only, no SYNCBUSY |
| 0x02 | CFGA | REFNUM[7:0] (bits 7:0), DIVREF (bit 15, N-series only) — enable-protected, PAC-protected |
| 0x04–0x07 | Reserved | — |
| 0x08 | INTENCLR | DONE (bit 0) — PAC-protected |
| 0x09 | INTENSET | DONE (bit 0) — PAC-protected |
| 0x0A | INTFLAG | DONE (bit 0) — cleared by writing 1 |
| 0x0B | STATUS | OVF (bit 1, sticky R/W), BUSY (bit 0, read-only) |
| 0x0C | SYNCBUSY | ENABLE (bit 1), SWRST (bit 0) — 32-bit read-only |
| 0x10 | VALUE | VALUE[23:0] — 24-bit read-only measurement result |

## Concepts Mentioned

- [[FREQM Configuration]] — practical init and frequency calculation
- [[Clock System]] — GCLK PCHCTRL[3] and PCHCTRL[4] configuration
- [[NVIC Interrupt Map]] — FREQM on line 4

## See Also

- [[FREQM Configuration]] — concept page with init code
- [[Clock System]] — GCLK setup for reference and measured clocks
- [[NVIC Interrupt Map]] — FREQM interrupt line
- [[SAMC21 Datasheet Ch.12 Peripherals Configuration Summary]] — FREQM clock indexes
