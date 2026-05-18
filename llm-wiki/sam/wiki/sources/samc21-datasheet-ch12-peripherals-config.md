---
title: SAMC21 Datasheet Ch.12 Peripherals Configuration Summary
type: source
tags: [peripheral, address, gclk, apb, ahb, events, dma, samc21, datasheet]
sources: [samc21-datasheet-ch12-peripherals-config]
created: 2026-05-05
updated: 2026-05-18
---

# SAMC21 Datasheet Ch.12 Peripherals Configuration Summary

Complete per-peripheral table: base address, IRQ line, AHB/APB clock indexes,
GCLK PCHCTRL indexes, PAC index, event user/generator indexes, DMA trigger
indexes, and sleep-walking support.

## Abstract

Chapter 12 provides four variant-specific tables (SAM C21 N, SAM C20 N,
SAM C21 E/G/J, SAM C20 E/G/J) in a single consolidated view. Each row lists
every parameter needed to enable and use a peripheral from scratch, covering
clock enable, GCLK mux, PAC numbering, event connectivity, and DMA.

## Key Facts

- APB-A peripheral clocks: **all enabled at reset** (PM, MCLK, RSTC, OSCCTRL,
  OSC32KCTRL, SUPC, GCLK, WDT, RTC, EIC, FREQM).
- APB-B: PORT, DSU, NVMCTRL clocks enabled at reset; MTB not clocked.
- APB-C and APB-D clocks: **NOT enabled at reset** — firmware must set
  `MCLK->APBCMASK.reg |= MCLK_APBCMASK_<PERIPHERAL>` before first access.
- DMAC: has AHB clock enabled at reset (AHB Index 7); no APB clock needed.
- CAN0/CAN1: AHB clocks (not APB), not enabled at reset.
- TSENS: APB clock not enabled at reset (APB-A index 12, N=No).
- GCLK PCHCTRL index is used in `GCLK->PCHCTRL[n]` to connect a generator.
- Shared GCLK slots: TC0/TC1 share PCHCTRL[30]; TC2/TC3 share PCHCTRL[31];
  TCC0/TCC1 share PCHCTRL[28]; all SERCOMs share SLOW clock on PCHCTRL[18]
  (except SERCOM4/5 which share PCHCTRL[24] for SLOW).

## AHB-APB Bridge A Peripherals (SAM C21, all variants)

| Peripheral | Base Address | IRQ | APB Index | APB@Reset | GCLK PCHCTRL |
|-----------|-------------|-----|-----------|-----------|-------------|
| PAC | 0x40000000 | 0 | 0 | Y | — |
| PM | 0x40000400 | 0 | 1 | Y | — |
| MCLK | 0x40000800 | 0 | 2 | Y | — |
| RSTC | 0x40000C00 | — | 3 | Y | — |
| OSCCTRL | 0x40001000 | 0 | 4 | Y | 0 (FDPLL src), 1 (FDPLL 32kHz) |
| OSC32KCTRL | 0x40001400 | 0 | 5 | Y | — |
| SUPC | 0x40001800 | 0 | 6 | Y | — |
| GCLK | 0x40001C00 | — | 7 | Y | — |
| WDT | 0x40002000 | 1 | 8 | Y | — |
| RTC | 0x40002400 | 2 | 9 | Y | — |
| EIC | 0x40002800 | 3/NMI | 10 | Y | 2 |
| FREQM | 0x40002C00 | 4 | 11 | Y | 3 (MSR), 4 (REF) |
| TSENS | 0x40003000 | 5 | 12 | **N** | 5 |

## AHB-APB Bridge B Peripherals

| Peripheral | Base Address | IRQ | AHB Index | APB@Reset | GCLK PCHCTRL |
|-----------|-------------|-----|-----------|-----------|-------------|
| PORT | 0x41000000 | — | — | Y (APB) | — |
| DSU | 0x41002000 | — | 3 Y | Y (APB) | — |
| NVMCTRL | 0x41004000 | 6 | 5 Y | Y (APB) | 39 |
| DMAC | 0x41006000 | 7 | 7 Y | — | — |
| MTB | 0x41008000 | — | — | — | — |

## AHB-APB Bridge C Peripherals (SAM C21, all variants)

All APB-C clocks are **disabled at reset** — must be enabled in `MCLK->APBCMASK`.

| Peripheral | Base Address | IRQ | APB Index | GCLK PCHCTRL | DMA |
|-----------|-------------|-----|-----------|-------------|-----|
| EVSYS | 0x42000000 | 8 | 0 | 6–17 (one per channel) | — |
| SERCOM0 | 0x42000400 | 9 | 1 | 19 CORE, 18 SLOW | 2 RX, 3 TX |
| SERCOM1 | 0x42000800 | 10 | 2 | 20 CORE, 18 SLOW | 4 RX, 5 TX |
| SERCOM2 | 0x42000C00 | 11 | 3 | 21 CORE, 18 SLOW | 6 RX, 7 TX |
| SERCOM3 | 0x42001000 | 12 | 4 | 22 CORE, 18 SLOW | 8 RX, 9 TX |
| SERCOM4 | 0x42001400 | 13 | 5 | 23 CORE, 24 SLOW | 10 RX, 11 TX |
| SERCOM5 | 0x42001800 | 14 | 6 | 25 CORE, 24 SLOW | 12 RX, 13 TX |
| CAN0 | 0x42001C00 | 15 | — | 26 | — |
| CAN1 | 0x42002000 | 16 | — | 27 | — |
| TCC0 | 0x42002400 | 17 | 9 | 28 | 16–20: OVF/MC0-3 |
| TCC1 | 0x42002800 | 18 | 10 | 28 (shared w/ TCC0) | 21–23: OVF/MC0-1 |
| TCC2 | 0x42002C00 | 19 | 11 | 29 | 24–26: OVF/MC0-1 |
| TC0 | 0x42003000 | 20 | 12 | 30 | 27–29: OVF/MC0-1 |
| TC1 | 0x42003400 | 21 | 13 | 30 (shared w/ TC0) | 30–32: OVF/MC0-1 |
| TC2 | 0x42003800 | 22 | 14 | 31 | 33–35: OVF/MC0-1 |
| TC3 | 0x42003C00 | 23 | 15 | 31 (shared w/ TC2) | 36–38: OVF/MC0-1 |
| TC4 | 0x42004000 | 24 | 16 | 32 | 39–41: OVF/MC0-1 |
| ADC0 | 0x42004400 | 25 | 17 | 33 | 42: RESRDY |
| ADC1 | 0x42004800 | 26 | 18 | 34 | 43: RESRDY |
| SDADC | 0x42004C00 | 29 | 19 | 35 | 44: RESRDY |
| AC | 0x42005000 | 27 | 20 | 40 on ATSAMC21J18A (`AC_GCLK_ID`) | — |
| DAC | 0x42005400 | 28 | 21 | 36 | 45: EMPTY |
| PTC | 0x42005800 | 30 | 22 | 37 | EOC: 46, WCOMP: 47, SEQ: 48 |
| CCL | 0x42005C00 | — | 23 | 38 | — |

Note: for the target ATSAMC21J18A, the local CMSIS header defines
`AC_GCLK_ID = 40`. Do not use the older wiki value `PCHCTRL[34]` or the old
ADC1-clock workaround for this project. If targeting a different device/package,
verify the generated CMSIS `AC_GCLK_ID` macro before writing `PCHCTRL`.

## AHB-APB Bridge D Peripherals (SAM C21 N only)

| Peripheral | Base Address | IRQ | APB Index | GCLK PCHCTRL | DMA |
|-----------|-------------|-----|-----------|-------------|-----|
| SERCOM6 | 0x43000000 | 9 | 0 | 41 CORE, 18 SLOW | 49 RX, 50 TX |
| SERCOM7 | 0x43000400 | 10 | 1 | 42 CORE, 18 SLOW | 51 RX, 52 TX |
| TC5 | 0x43000800 | 20 | 2 | 43 | 53–55: OVF/MC0-1 |
| TC6 | 0x43000C00 | 21 | 3 | 44 | 56–58: OVF/MC0-1 |
| TC7 | 0x43001000 | 22 | 4 | 45 | 59–61: OVF/MC0-1 |
| DIVAS | 0x48000000 | — | — (AHB 12) | — | — |

## Selected Event Generator Indexes (SAM C21)

| Generator Index | Source |
|----------------|--------|
| 1 | OSCCTRL: XOSC_FAIL |
| 2 | OSC32KCTRL: XOSC32K_FAIL |
| 3–13 | RTC: CMP0/ALARM0–ALARM1, OVF, PER0–7 |
| 14–29 | EIC: EXTINT0–15 (+ NMI) |
| 30 | TSENS: WINMON |
| 31–34 | DMAC: CH0–3 |
| 35–38 | OSCCTRL/OSC32KCTRL (FDPLL events) |
| 52–66 | TC0–TC4: OVF/MC0/MC1 |
| 67–70 | ADC0/ADC1: RESRDY/WINMON |
| 71–72 | SDADC: RESRDY/WINMON |
| 73–78 | AC: COMP0–3, WIN0–1 |
| 79 | DAC: EMPTY |
| 80–81 | PTC: EOC, WCOMP |
| 82–85 | CCL: LUTOUT0–3 |
| 86 | PAC: ACCERR |

## Concepts Mentioned

- [[Memory Map]] — peripheral base addresses
- [[Clock System]] — GCLK PCHCTRL numbering, MCLK APB masks
- [[DMA Controller]] — DMA trigger index table
- [[EVSYS]] — event generator/user index tables

## See Also

- [[Memory Map]] — physical address space and NVM areas
- [[Clock System]] — GCLK/MCLK enable sequence
- [[DMA Controller]] — how to use DMA trigger indexes
- [[EVSYS]] — how to connect event generators to users
