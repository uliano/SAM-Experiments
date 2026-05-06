---
title: SAMC21 Datasheet Chapter 17 — MCLK
type: source
tags: [mclk, clock, samc21, datasheet]
date: 2026-05-05
---

# SAMC21 Datasheet Ch. 17 — MCLK Main Clock

Source: SAM C20/C21 Data Sheet DS60001479D, chapter 17, pages 154–173.

## Overview

MCLK takes GCLK_MAIN from Generator 0 and distributes synchronous clocks to:
- CPU (CLK_CPU)
- AHB bus (CLK_AHBx)
- APB buses A/B/C (CLK_APBx)

Base address: 0x40000800 (APB-A). Always enabled, cannot be reset.

## Register Map

| Offset | Name | Reset | Description |
|--------|------|-------|-------------|
| 0x00 | CTRLA | 0x00 | Control A (all reserved) |
| 0x01 | INTENCLR | 0x00 | Interrupt Enable Clear (bit 0 = CKRDY) |
| 0x02 | INTENSET | 0x00 | Interrupt Enable Set (bit 0 = CKRDY) |
| 0x03 | INTFLAG | 0x01 | Interrupt Flag (bit 0 = CKRDY, reset=1) |
| 0x05 | CPUDIV | 0x01 | CPU Clock Division |
| 0x10 | AHBMASK | 0x000003CFF | AHB Clock Mask |
| 0x14 | APBAMASK | 0x00000FFF | APB-A Clock Mask |
| 0x18 | APBBMASK | 0x00000007 | APB-B Clock Mask |
| 0x1C | APBCMASK | 0x00000000 | APB-C Clock Mask |
| 0x20 | APBDMASK | — | APB-D (N-series only) |

## CPUDIV Register (offset 0x05)

8-bit, PAC Write-Protected. Sets the prescaler between GCLK_MAIN and CLK_CPU.

| Value | Divide By |
|-------|----------|
| 0x01 | 1 (default — CPU = GCLK_MAIN) |
| 0x02 | 2 |
| 0x04 | 4 |
| 0x08 | 8 |
| 0x10 | 16 |
| 0x20 | 32 |
| 0x40 | 64 |
| 0x80 | 128 |

Other values are reserved (written but flagged as violations by PAC).

## AHBMASK Register (offset 0x10, reset=0x000003CFF)

Controls AHB clock gating. A cleared bit stops the AHB clock to that slave.

| Bit | Name | Reset | Notes |
|-----|------|-------|-------|
| 13 | APBD | 1 | N-series only |
| 12 | DIVAS | 1 | Hardware divider |
| 10 | PAC | 1 | Peripheral Access Controller |
| 9 | CAN1 | 0 | CAN1 (G/J/N only) |
| 8 | CAN0 | 0 | CAN0 |
| 7 | DMAC | 1 | DMA Controller |
| 6 | HSRAM | 1 | High-speed SRAM |
| 5 | NVMCTRL | 1 | Flash controller |
| 4 | HMATRIXHS | 1 | Bus matrix |
| 3 | DSU | 1 | Debug Service Unit |
| 2 | APBC | 1 | APB-C bridge |
| 1 | APBB | 1 | APB-B bridge |
| 0 | APBA | 1 | APB-A bridge |

**Warning:** Do not disable NVMCTRL or the MCLK/APB bridge — doing so makes those registers inaccessible and requires a system reset.

## APBAMASK Register (offset 0x14, reset=0x00000FFF)

Controls APB-A peripheral clocks. Bits 0–11 enabled by default.

| Bit | Peripheral | Reset |
|-----|-----------|-------|
| 12 | TSENS | 0 |
| 11 | FREQM | 1 |
| 10 | EIC | 1 |
| 9 | RTC | 1 |
| 8 | WDT | 1 |
| 7 | GCLK | 1 |
| 6 | SUPC | 1 |
| 5 | OSC32KCTRL | 1 |
| 4 | OSCCTRL | 1 |
| 3 | RSTC | 1 |
| 2 | MCLK | 1 |
| 1 | PM | 1 |
| 0 | PAC | 1 |

## APBBMASK Register (offset 0x18, reset=0x00000007)

| Bit | Peripheral | Reset |
|-----|-----------|-------|
| 5 | HMATRIXHS | 0 |
| 2 | NVMCTRL | 1 |
| 1 | DSU | 1 |
| 0 | PORT | 1 |

## APBCMASK Register (offset 0x1C, reset=0x00000000)

All disabled by default. Enable each peripheral before accessing its registers.

| Bit | Peripheral | Bit | Peripheral |
|-----|-----------|-----|-----------|
| 23 | CCL | 12 | TC0 |
| 22 | PTC | 11 | TCC2 |
| 21 | DAC | 10 | TCC1 |
| 20 | AC | 9 | TCC0 |
| 19 | SDADC | 6 | SERCOM5 |
| 18 | ADC1 | 5 | SERCOM4 |
| 17 | ADC0 | 4 | SERCOM3 |
| 16 | TC4 | 3 | SERCOM2 |
| 15 | TC3 | 2 | SERCOM1 |
| 14 | TC2 | 1 | SERCOM0 |
| 13 | TC1 | 0 | EVSYS |

## CKRDY Interrupt

INTFLAG.CKRDY resets to 1 (clocks ready after power-on). After writing CPUDIV, the flag clears until the new clock is stable. If INTENSET.CKRDY=1, an interrupt fires when the new clock is ready. Never re-write CPUDIV while INTFLAG.CKRDY=0.

## Typical Peripheral Enable Pattern

```cpp
// Enable SERCOM5 APB clock
MCLK->APBCMASK.reg |= MCLK_APBCMASK_SERCOM5;

// Enable DMAC AHB clock
MCLK->AHBMASK.reg |= MCLK_AHBMASK_DMAC;

// Enable TC0+TC1 APB clocks (for 32-bit paired mode)
MCLK->APBCMASK.reg |= MCLK_APBCMASK_TC0 | MCLK_APBCMASK_TC1;
```

## See Also

- [[Clock System]] — full three-layer architecture
- [[GCLK]] — generic clock generators and peripheral channels
- [[DMA Controller]] — needs MCLK_AHBMASK_DMAC
- [[Uart DMA]] / [[Uart INT]] — need MCLK_APBCMASK_SERCOM5
- [[TC 32-Bit Paired Mode]] — needs TC0+TC1 or TC2+TC3 APB clocks
