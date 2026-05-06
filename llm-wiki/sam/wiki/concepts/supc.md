---
title: SUPC
type: concept
tags: [supc, bod, vreg, vref, power, firmware, samc21]
sources: [samc21-datasheet-ch22-supc]
created: 2026-05-05
updated: 2026-05-05
---

# SUPC

Supply Controller on the SAMC21. Manages brown-out detection on VDD (BODVDD)
and VDDCORE (BODCORE), the internal voltage regulator (VREG), and the
internal voltage reference (VREF) for ADC/SDADC/DAC.

## BODVDD — Brown-Out Detector for VDD

Monitors the VDD supply against a programmable threshold. Reset values loaded
from NVM User Row; factory defaults are typically appropriate.

```cpp
// Read NVM User Row for factory-set threshold before overriding
uint32_t bod = SUPC->BODVDD.reg;

// Reconfigure (enable-protected: must disable first)
SUPC->BODVDD.reg &= ~SUPC_BODVDD_ENABLE;
SUPC->BODVDD.reg = SUPC_BODVDD_LEVEL(39)       // example: ~3.0 V threshold
                 | SUPC_BODVDD_ACTION_RESET
                 | SUPC_BODVDD_HYST
                 | SUPC_BODVDD_ENABLE;
while (!(SUPC->STATUS.reg & SUPC_STATUS_BVDDSRDY));
```

### BODVDD Key Fields

| Field | Description |
|-------|-------------|
| LEVEL[5:0] | Threshold 0x00–0x3F; each step ≈ 34 mV above ~1.7 V base |
| ACTION[1:0] | NONE(0) / RESET(1) / INT(2) |
| HYST | Hysteresis enable (recommended) |
| RUNSTDBY | Keep BOD running in standby |
| ACTCFG | 0=continuous, 1=sampled (lower current) |

### BODVDD Status Flags

| Flag | Condition |
|------|-----------|
| BODVDDRDY | Comparator ready after enable (detection valid) |
| BODVDDDET | Brown-out currently detected |
| BVDDSRDY | Ready after ENABLE write (same as BODVDDRDY in STATUS) |

Wait `SUPC_STATUS_BVDDSRDY` after enabling before trusting `BODVDDDET`.

## BODCORE — Brown-Out Detector for VDDCORE

Factory-calibrated, not user-configurable. Automatically disabled in standby
(VDDCORE is maintained by LP regulator; BODCORE does not apply). No software
interface beyond observing RSTC.RCAUSE.BODCORE after a reset.

## VREG — Voltage Regulator

```cpp
// VREG.ENABLE must always remain 1 — never clear it
// To select LP mode in standby, use PM->STDBYCFG.VREGSMOD or VREG.RUNSTDBY
SUPC->VREG.reg = SUPC_VREG_ENABLE;   // default; usually no need to touch
```

**VREG.ENABLE = 1 must never be cleared.** The internal regulator provides
VDDCORE; clearing ENABLE stops the chip.

## VREF — Internal Voltage Reference

Used as reference input for ADC, SDADC, and DAC.

```cpp
// 2.048 V reference, on-demand (powers up when ADC triggers)
SUPC->VREF.reg = SUPC_VREF_SEL_2V048 | SUPC_VREF_ONDEMAND;
```

### VREF Voltage Options

| SEL value | Voltage |
|-----------|---------|
| 0x0 | 1.024 V |
| 0x1 | Reserved |
| 0x2 | 2.048 V |
| 0x3 | 4.096 V |

ONDEMAND=1 is recommended: the reference starts automatically when a
peripheral requests a conversion and powers off afterwards.

VREFOE=1 routes the reference to the VREFP/VREFA pin for external use.

## Key Facts

- BODVDD defaults come from NVM User Row — read before overwriting; factory
  calibration may already be correct.
- BODCORE is invisible to software; its reset source appears only in RCAUSE.
- VREG.ENABLE must always be 1.
- VREF SEL=0x1 is reserved; always use 0x0, 0x2, or 0x3.
- VREF ONDEMAND=1 saves current and is the recommended default.
- After enabling BODVDD, firmware must wait STATUS.BVDDSRDY before checking
  BODVDDDET, otherwise may see a false detection.

## See Also

- [[SAMC21 Datasheet Ch.22 SUPC]] — full register reference
- [[RSTC]] — BODVDD/BODCORE appear in RCAUSE after a reset
- [[PM]] — sleep mode / regulator interaction
- [[Memory Map]] — SUPC base address
- [[NVIC Interrupt Map]] — SUPC on shared IRQ line 0
