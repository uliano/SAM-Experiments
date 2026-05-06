---
title: SAMC21 Datasheet Chapter 22 — SUPC
type: source
tags: [supc, bod, vreg, vref, power, samc21, datasheet]
date: 2026-05-05
---

# SAMC21 Datasheet Ch. 22 — SUPC – Supply Controller

Source: SAM C20/C21 Data Sheet DS60001479D, chapter 22, pages 249–263.

## Overview

The Supply Controller (SUPC) monitors supply voltages and manages the on-chip
voltage regulators and reference. It contains:
- **BODVDD**: Brown-out detector for the VDD supply (user-configurable, NVM-loaded defaults)
- **BODCORE**: Brown-out detector for VDDCORE (factory-calibrated, not configurable)
- **VREG**: Internal voltage regulator (must always be enabled)
- **VREF**: Internal voltage reference for ADC/SDADC/DAC

## Register Map (base in APBA)

| Offset | Name | Reset | Description |
|--------|------|-------|-------------|
| 0x00 | INTENCLR | 0x00 | Interrupt enable clear |
| 0x04 | INTENSET | 0x00 | Interrupt enable set |
| 0x08 | INTFLAG | 0x00 | BVDDSRDY(2)/BODVDDDET(1)/BODVDDRDY(0) |
| 0x0C | STATUS | 0x00 | BVDDSRDY(2)/BODVDDDET(1)/BODVDDRDY(0) — read-only |
| 0x10 | BODVDD | (NVM) | Brown-out detector for VDD — see below |
| 0x18 | VREG | 0x02 | RUNSTDBY(6)/ENABLE(1, must stay 1) |
| 0x1C | VREF | 0x00 | SEL[19:16]/ONDEMAND(7)/RUNSTDBY(6)/VREFOE(2) |

## BODVDD Register (0x10)

Enable-protected and write-synchronized. Reset values loaded from NVM User Row.

| Bits | Name | Description |
|------|------|-------------|
| 21:16 | LEVEL[5:0] | Threshold level (0x00–0x3F); loaded from NVM User Row |
| 15:12 | PSEL[3:0] | Prescaler for BOD sampling clock |
| 8 | ACTCFG | Active config: 0=continuous, 1=sampled |
| 6 | RUNSTDBY | Keep BOD running in standby |
| 5 | STDBYCFG | Standby config: 0=continuous, 1=sampled |
| 4:3 | ACTION[1:0] | Action on detect: NONE(0)/RESET(1)/INT(2) |
| 2 | HYST | Hysteresis enable |
| 1 | ENABLE | Enable BODVDD |

### BODVDD Write Sequence

BODVDD is enable-protected: must disable before changing LEVEL or ACTION.
After enabling, wait STATUS.BVDDSRDY=1 before the detection result is valid.

```cpp
SUPC->BODVDD.reg &= ~SUPC_BODVDD_ENABLE;
// ... modify LEVEL/ACTION ...
SUPC->BODVDD.reg |= SUPC_BODVDD_ENABLE;
while (!(SUPC->STATUS.reg & SUPC_STATUS_BVDDSRDY));
```

## BODCORE

Factory-calibrated brown-out detector for VDDCORE (internal regulated supply).
Not user-configurable. Always disabled in standby (BODCORE does not monitor
VDDCORE in standby because VDDCORE is managed by VREG/LP regulator). No
BODCORE fields in any register exposed to software.

## VREG Register (0x18)

| Bit | Name | Description |
|-----|------|-------------|
| 6 | RUNSTDBY | Keep regulator in normal mode in standby (vs. LP mode) |
| 1 | ENABLE | Must always be 1; clearing disables the internal supply — chip stops |

**VREG.ENABLE must never be cleared.** The internal voltage regulator powers
the core; clearing ENABLE is destructive.

## VREF Register (0x1C)

Provides the internal bandgap voltage reference used by ADC, SDADC, and DAC.

| Bits / Bit | Name | Description |
|-----------|------|-------------|
| 19:16 | SEL[3:0] | Reference voltage: 1.024V(0x0) / 2.048V(0x2) / 4.096V(0x3) |
| 7 | ONDEMAND | Enable reference only when requested by peripheral |
| 6 | RUNSTDBY | Keep reference running in standby |
| 2 | VREFOE | Output VREF to VREFP/VREFA pin |

### VREF Enable Sequence

```cpp
SUPC->VREF.reg = SUPC_VREF_SEL_2V048 | SUPC_VREF_ONDEMAND;
// reference is now available on-demand to ADC/SDADC/DAC
```

## Interrupt / Status Flags

| Bit | Name | Condition |
|-----|------|-----------|
| 0 | BODVDDRDY | BODVDD ready (detection valid) |
| 1 | BODVDDDET | Brown-out detected on VDD |
| 2 | BVDDSRDY | BODVDD comparator ready after enable |

## Key Facts

- BODVDD reset values come from NVM User Row — factory defaults may already
  be appropriate; read before overwriting.
- BODCORE is not configurable and is automatically disabled in standby.
- VREG.ENABLE must always remain 1; never write 0.
- VREF SEL=0x1 is reserved; use 0x0 (1.024V), 0x2 (2.048V), or 0x3 (4.096V).
- VREF ONDEMAND=1 is recommended: reference powers up automatically when ADC
  is triggered, powers down after conversion — saves current.
- After enabling BODVDD, firmware must wait STATUS.BVDDSRDY=1 before trusting
  BODVDDDET status.

## See Also

- [[SUPC]] — firmware BODVDD/VREF configuration patterns
- [[PM]] — sleep modes and regulator interaction
- [[RSTC]] — BODVDD/BODCORE reset sources in RCAUSE
- [[Memory Map]] — SUPC base address
- [[NVIC Interrupt Map]] — SUPC IRQn (shared on line 0)
