---
title: Memory Map
type: concept
tags: [memory, flash, sram, nvm, samc21]
sources: [samc21-datasheet-ch09-memories]
created: 2026-05-05
updated: 2026-05-05
---

# Memory Map

Physical address layout for the SAMC21J18A (x18 variant: 256 KB Flash, 32 KB SRAM).

## Physical Address Space

| Region | Start | Size (x18) | Notes |
|--------|-------|-----------|-------|
| Embedded Flash | 0x00000000 | 256 KB | 4096 pages × 64 bytes |
| Embedded RWW Section | 0x00400000 | 8 KB | Read-while-write (EEPROM emulation) |
| SRAM | 0x20000000 | 32 KB | Single-cycle access |
| AHB-APB Bridge A | 0x40000000 | 64 KB | PM, MCLK, RSTC, OSCCTRL, OSC32KCTRL, SUPC, GCLK, WDT, RTC, EIC, FREQM, PAC |
| AHB-APB Bridge B | 0x41000000 | 64 KB | DSU, NVMCTRL, PORT, DMAC, MTB, EVSYS |
| AHB-APB Bridge C | 0x42000000 | 64 KB | SERCOM0-5, TCC0-2, TC0-4, ADC0-1, AC, DAC, SDADC, PTC, CCL, CAN0-1, TSENS |
| AHB-APB Bridge D | 0x43000000 | 64 KB | N-series only |
| AHB DIVAS | 0x48000000 | 64 KB | Hardware divider/square root |
| IOBUS | 0x60000000 | 64 KB | Single-cycle PORT access |

## Cortex-M0+ Core Peripherals

| Address | Peripheral |
|---------|-----------|
| 0xE000E000 | System Control Space (SCS) |
| 0xE000E010 | System Timer (SysTick) |
| 0xE000E100 | NVIC |
| 0xE000ED00 | System Control Block (SCB) |
| 0x41008000 | Micro Trace Buffer (MTB) |

## Flash Parameters (x18 = SAMC21J18A)

- Size: 256 KB
- Pages: 4096 × 64 bytes
- RWW section: 8 KB (128 pages × 64 bytes) at 0x00400000

## NVM Special Areas

### User Row — 0x00804000

64-bit (two 32-bit words) read at power-on. Write via NVMCTRL. Changes take effect after reset.

| Bits | Name | Production Default | Notes |
|------|------|-------------------|-------|
| 2:0 | BOOTPROT | 0x7 | Bootloader protection size |
| 6:4 | EEPROM | 0x7 | EEPROM emulation area size |
| 13:8 | BODVDD Level | 0x8 | Brown-out threshold |
| 14 | BODVDD Disable | 0x0 | 0 = enabled |
| 16:15 | BODVDD Action | 0x1 | Reset/interrupt on BOD event |
| 25:17 | BODCORE cal | 0xA8 | **DO NOT CHANGE** — production calibration |
| 26 | WDT Enable | 0x0 | WDT enabled at power-on |
| 27 | WDT Always-On | 0x0 | |
| 31:28 | WDT Period | 0xB | → WDT.CONFIG.PER |
| 35:32 | WDT Window | 0xB | → WDT.CONFIG.WINDOW |
| 39:36 | WDT EWOFFSET | 0xB | → WDT.EWCTRL.EWOFFSET |
| 40 | WDT WEN | 0x0 | Window mode enable |
| 41 | BODVDD Hysteresis | 0x0 | → SUPC.BODVDD.HYSTERESIS |
| 42 | BODCORE cal | 0x0 | **DO NOT CHANGE** |
| 63:48 | LOCK | 0xFFFF | NVM region lock bits → NVMCTRL.LOCK |

### Software Calibration Area — 0x00806020

Read-only. Must be read by application and written to calibration registers.

| Bits | Name | Target Register |
|------|------|----------------|
| 2:0 | ADC0 LINEARITY | ADC0 CALIB.BIASREFBUF |
| 5:3 | ADC0 BIASCAL | ADC0 CALIB.BIASCOMP |
| 8:6 | ADC1 LINEARITY | ADC1 CALIB.BIASREFBUF |
| 11:9 | ADC1 BIASCAL | ADC1 CALIB.BIASCOMP |
| 18:12 | OSC32K CAL | OSC32KCTRL.OSC32K.CALIB |
| 40:19 | CAL48M 5V | OSCCTRL.CAL48M[21:0] (VDD 3.6V–5.5V) |
| 62:41 | CAL48M 3V3 | OSCCTRL.CAL48M[21:0] (VDD 2.7V–3.6V) |

Select CAL48M 5V or 3V3 based on the actual supply voltage range.

### Temperature Calibration Area — 0x00806030

Read-only. SAM C21 only (not SAM C20).

| Bits | Name | Target Register |
|------|------|----------------|
| 5:0 | TSENS TCAL | TSENS CAL.TCAL |
| 11:6 | TSENS FCAL | TSENS CAL.FCAL |
| 35:12 | TSENS GAIN | TSENS GAIN |
| 59:36 | TSENS OFFSET | TSENS OFFSET |

### Device Serial Number — 128-bit unique ID

Guaranteed unique only when all 128 bits are used together:

```cpp
uint32_t sn[4];
sn[0] = *(volatile uint32_t*)0x0080A00C;
sn[1] = *(volatile uint32_t*)0x0080A040;
sn[2] = *(volatile uint32_t*)0x0080A044;
sn[3] = *(volatile uint32_t*)0x0080A048;
```

## Peripheral Base Addresses (APB-C, partial)

| Peripheral | Base Address |
|-----------|-------------|
| SERCOM0 | 0x42000400 |
| SERCOM1 | 0x42000800 |
| SERCOM2 | 0x42000C00 |
| SERCOM3 | 0x42001000 |
| SERCOM4 | 0x42001400 |
| SERCOM5 | 0x42001800 |
| TCC0 | 0x42002000 |
| TCC1 | 0x42002400 |
| TCC2 | 0x42002800 |
| TC0 | 0x42003000 |
| TC1 | 0x42003400 |
| TC2 | 0x42003800 |
| TC3 | 0x42003C00 |
| TC4 | 0x42004000 |

## See Also

- [[NVIC Interrupt Map]] — interrupt line assignments
- [[Clock System]] — GCLK/MCLK base addresses
- [[DMA Controller]] — DMAC at 0x41004800
