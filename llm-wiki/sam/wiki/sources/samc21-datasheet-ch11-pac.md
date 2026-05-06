---
title: SAMC21 Datasheet Ch.11 PAC
type: source
tags: [pac, peripheral-access-controller, write-protection, samc21, datasheet]
sources: [samc21-datasheet-ch11-pac]
created: 2026-05-05
updated: 2026-05-05
---

# SAMC21 Datasheet Ch.11 PAC — Peripheral Access Controller

Peripheral Access Controller for the SAM C20/C21. Provides optional write
protection for peripheral registers and detects unauthorized accesses.

## Abstract

The PAC controls write access to registers of each peripheral. It is always
enabled and its state is only reset by a hardware reset (not user SWRST). Write
protection can be individually set, cleared, or locked per peripheral using the
WRCTRL register. Error events and interrupts are generated for unauthorized
accesses.

## Key Facts

- Base address: 0x40000000 (APB-A Bridge, PAC index 0, PERID = 0).
- PAC is always enabled. No GCLK needed. No MCLK mask.
- State only reset by hardware (power-on or external) reset, not by user SWRST.
- WRCTRL.KEY values: 0x0=OFF (no action), 0x1=CLEAR (unprotect), 0x2=SET (protect), 0x3=LOCK (protect + lock until hardware reset).
- PERID formula: `PERID = 32 × BridgeNumber + N` where N is the peripheral index from the Ch.12 "PAC, Index" column.
  - Bridge A (APB-A): PERID 0+N (PAC, PM, MCLK, RSTC, OSCCTRL, OSC32KCTRL, SUPC, GCLK, WDT, RTC, EIC, FREQM, TSENS)
  - Bridge B (APB-B): PERID 32+N (PORT, DSU, NVMCTRL, DMAC, MTB)
  - Bridge C (APB-C): PERID 64+N (EVSYS, SERCOM0–5, TCC0–2, TC0–4, ADC0–1, SDADC, AC, DAC, PTC, CCL, CAN0–1)
  - Bridge D (APB-D): PERID 96+N (SERCOM6–7, TC5–7)
- Error types: unauthorized write, unauthorized erase (NVM), unauthorized read (NVM), write-lock attempt.
- EVCTRL.ERREO: generates a system event on any access error.
- INTENCLR/INTENSET.ERR: enable/disable PAC error interrupt.

## Register Summary

| Offset | Name | Key Fields |
|--------|------|-----------|
| 0x00 | WRCTRL | KEY[23:16] (0x0/1/2/3), PERID[15:0] |
| 0x04 | EVCTRL | ERREO (bit 0) — event output on error |
| 0x08 | INTENCLR | ERR (bit 0) — PAC Write-Protected |
| 0x09 | INTENSET | ERR (bit 0) — PAC Write-Protected |
| 0x10 | INTFLAGAHB | HPB3(8), DIVAS(7), LPRAMDMAC(6), HPB2(5), HPB0(4), HPB1(3), HSRAMDSU(2), HSRAMCM0P(1), FLASH(0) |
| 0x14 | INTFLAGA | TSENS(12), FREQM(11), EIC(10), RTC(9), WDT(8), GCLK(7), SUPC(6), OSC32KCTRL(5), OSCCTRL(4), RSTC(3), MCLK(2), PM(1), PAC(0) |
| 0x18 | INTFLAGB | MTB(4), DMAC(3), NVMCTRL(2), DSU(1), PORT(0) |
| 0x1C | INTFLAGC | CCL(23), PTC(22), DAC(21), AC(20), SDADC(19), ADC1(18), ADC0(17), TC4(16), TC3(15), TC2(14), TC1(13), TC0(12), TCC2(11), TCC1(10), TCC0(9), CAN1(8), CAN0(7), SERCOM5(6), SERCOM4(5), SERCOM3(4), SERCOM2(3), SERCOM1(2), SERCOM0(1), EVSYS(0) |
| 0x20 | INTFLAGD | TC7(4), TC6(3), TC5(2), SERCOM7(1), SERCOM6(0) |
| 0x34 | STATUSA | Same bit layout as INTFLAGA — read-only write-protection status for Bridge A |
| 0x38 | STATUSB | Same bit layout as INTFLAGB — Bridge B write-protection status |
| 0x3C | STATUSC | Same bit layout as INTFLAGC — Bridge C write-protection status |
| 0x40 | STATUSD | Same bit layout as INTFLAGD — Bridge D write-protection status |

All INTFLAG bits are cleared by writing 1 (write 0 has no effect).
All STATUS bits are read-only; writing has no effect.

## Concepts Mentioned

- [[PAC Configuration]] — WRCTRL protect/unprotect sequences, PERID lookup
- [[Memory Map]] — PAC base address in APB-A address space

## See Also

- [[PAC Configuration]] — practical usage with code examples
- [[SAMC21 Datasheet Ch.12 Peripherals Configuration Summary]] — PAC, Index column for PERID lookup
- [[Memory Map]] — peripheral base addresses
