---
title: RSTC
type: concept
tags: [rstc, reset, firmware, samc21]
sources: [samc21-datasheet-ch18-rstc]
created: 2026-05-05
updated: 2026-05-05
---

# RSTC

Reset Controller on the SAMC21. Records the cause of the last reset in the
single RCAUSE register. Used during startup to distinguish boot sources
and select the appropriate recovery path.

## Reading Reset Cause

```cpp
uint8_t cause = RSTC->RCAUSE.reg;

if (cause & RSTC_RCAUSE_POR)    { /* power-on — full cold start */ }
if (cause & RSTC_RCAUSE_WDT)    { /* watchdog timeout or window violation */ }
if (cause & RSTC_RCAUSE_SYST)   { /* software-requested reset */ }
if (cause & RSTC_RCAUSE_EXT)    { /* RESET# pin asserted */ }
if (cause & RSTC_RCAUSE_BODVDD) { /* VDD brown-out */ }
if (cause & RSTC_RCAUSE_BODCORE){ /* VDDCORE brown-out */ }
```

Read RCAUSE before `sys_init()` alters the clock tree so the value is not
lost. Store it in a `volatile` global if needed after init.

## RCAUSE Bit Reference

| Bit | Name | Trigger |
|-----|------|---------|
| 0 | POR | VDD power-on ramp |
| 1 | BODCORE | VDDCORE brown-out (factory threshold) |
| 2 | BODVDD | VDD brown-out (SUPC BODVDD threshold) |
| 4 | EXT | External RESET# pin |
| 5 | WDT | Watchdog timeout or closed-window early clear |
| 6 | SYST | `SCB->AIRCR` SYSRESETREQ or `NVIC_SystemReset()` |

Only one bit is set at a time.

## Reset Scope

| Reset Class | Members | Scope |
|------------|---------|-------|
| Power supply | POR, BODCORE, BODVDD | Resets RTC, OSC32KCTRL, RSTC, GCLK-WRTLOCK only |
| User | EXT, WDT, SYST | Full chip reset — all peripherals, CPU, SRAM undefined |

After a power-supply reset, most peripheral registers retain their previous
values. Firmware that needs a clean state must explicitly reset peripherals
via their SWRST bits.

## Issuing a Software Reset

```cpp
NVIC_SystemReset();   // CMSIS helper — sets SYSRESETREQ, waits for reset
// or directly:
SCB->AIRCR = (0x05FA << SCB_AIRCR_VECTKEY_Pos) | SCB_AIRCR_SYSRESETREQ_Msk;
```

This produces a SYST user-class reset — full chip reset.

## Key Facts

- RCAUSE is read-only; bits are sticky until the next reset.
- No interrupts, no events, no DMA — RSTC is monitoring-only.
- Power-supply resets have limited scope; do not assume peripheral state is
  clean after BODVDD or BODCORE reset.

## See Also

- [[SAMC21 Datasheet Ch.18 RSTC]] — register reference
- [[WDT]] — watchdog reset source
- [[SUPC]] — BODVDD configuration
- [[Memory Map]] — RSTC base address
- [[NVIC Interrupt Map]] — no dedicated IRQ for RSTC
