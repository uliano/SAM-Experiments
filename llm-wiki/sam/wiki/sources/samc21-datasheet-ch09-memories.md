---
title: SAMC21 Datasheet Ch.9 Memories
type: source
tags: [memory, flash, sram, nvm, calibration, samc21, datasheet]
sources: [samc21-datasheet-ch09-memories]
created: 2026-05-05
updated: 2026-05-05
---

# SAMC21 Datasheet Ch.9 Memories

Physical memory map, Flash parameters, and NVM special areas for the SAM C20/C21.

## Abstract

Chapter 9 defines the complete 32-bit address space of the SAM C20/C21. It
covers the Flash and SRAM sizes across the four flash density variants (x15–x18),
the read-while-write (RWW) section, AHB-APB bridge windows, and the four NVM
special areas: User Row, Software Calibration Area, Temperature Calibration Area
(SAM C21 only), and the 128-bit Device Serial Number.

## Key Facts

- Flash at 0x00000000; SRAM at 0x20000000; AHB-APB bridges at 0x40000000–0x43000000.
- All flash variants use 64-byte pages. RWW section uses 64-byte pages too.
- N-series (100-pin) only available in x18 (256 KB) and x17 (128 KB) flash densities.
- AHB-APB Bridge D (0x43000000) exists only on x18 devices.
- NVM User Row auto-loaded at power-on; values affect BOD, WDT, BOOTPROT, LOCK.
- NVM Software Calibration Area read-only; must be read by application and written to peripheral registers.
- NVM Temperature Calibration Area present only on SAM C21 (not SAM C20).
- Serial number: 128-bit unique ID at four non-contiguous 32-bit addresses; all 128 bits required for uniqueness.

## Physical Memory Map

| Memory Region | Start Address | Size (x18) | Size (x17) | Size (x16) | Size (x15) |
|---------------|--------------|-----------|-----------|-----------|-----------|
| Embedded Flash | 0x00000000 | 256 KB | 128 KB | 64 KB | 32 KB |
| Embedded RWW Section | 0x00400000 | 8 KB | 4 KB | 2 KB | 1 KB |
| Embedded SRAM | 0x20000000 | 32 KB | 16 KB | 8 KB | 4 KB |
| AHB-APB Bridge A | 0x40000000 | 64 KB | 64 KB | 64 KB | 64 KB |
| AHB-APB Bridge B | 0x41000000 | 64 KB | 64 KB | 64 KB | 64 KB |
| AHB-APB Bridge C | 0x42000000 | 64 KB | 64 KB | 64 KB | 64 KB |
| AHB-APB Bridge D | 0x43000000 | 64 KB | — | — | — |
| AHB DIVAS | 0x48000000 | 64 KB | 64 KB | 64 KB | 64 KB |
| IOBUS | 0x60000000 | 64 KB | 64 KB | 64 KB | 64 KB |

## Flash Parameters

| Variant | Flash | Pages | RWW | RWW Pages |
|---------|-------|-------|-----|-----------|
| x18 | 256 KB | 4096 | 8 KB | 128 |
| x17 | 128 KB | 2048 | 4 KB | 64 |
| x16 | 64 KB | 1024 | 2 KB | 32 |
| x15 | 32 KB | 512 | 1 KB | 16 |

All page sizes: 64 bytes.

## NVM User Row — 0x00804000

Two 32-bit words (64 bits total). Auto-read at power-on; changes require device reset.
Write via NVMCTRL (see [[NVMCTRL]]). Read at any time.

| Bits | Name | Production Default | Target Register |
|------|------|-------------------|----------------|
| 2:0 | BOOTPROT | 0x7 | NA (NVMCTRL bootloader size) |
| 6:4 | EEPROM | 0x7 | NA (EEPROM emulation area size) |
| 13:8 | BODVDD Level | 0x8 | SUPC.BODVDD.LEVEL |
| 14 | BODVDD Disable | 0x0 | SUPC.BODVDD.ENABLE (0 = BOD enabled) |
| 16:15 | BODVDD Action | 0x1 | SUPC.BODVDD.ACTION |
| 25:17 | BODCORE calibration | 0xA8 | **DO NOT CHANGE** — production calibration |
| 26 | WDT Enable | 0x0 | WDT.CTRLA.ENABLE |
| 27 | WDT Always-On | 0x0 | WDT.CTRLA.ALWAYSON |
| 31:28 | WDT Period | 0xB | WDT.CONFIG.PER |
| 35:32 | WDT Window | 0xB | WDT.CONFIG.WINDOW |
| 39:36 | WDT EWOFFSET | 0xB | WDT.EWCTRL.EWOFFSET |
| 40 | WDT WEN | 0x0 | WDT.CTRLA.WEN |
| 41 | BODVDD Hysteresis | 0x0 | SUPC.BODVDD.HYSTERESIS |
| 42 | BODCORE calibration | 0x0 | **DO NOT CHANGE** |
| 63:48 | LOCK | 0xFFFF | NVMCTRL.LOCK (NVM region lock) |

## NVM Software Calibration Area — 0x00806020

Read-only. Must be read by firmware and written to peripheral calibration registers.

| Bits | Name | Target Register |
|------|------|----------------|
| 2:0 | ADC0 LINEARITY | ADC0 CALIB.BIASREFBUF |
| 5:3 | ADC0 BIASCAL | ADC0 CALIB.BIASCOMP |
| 8:6 | ADC1 LINEARITY | ADC1 CALIB.BIASREFBUF |
| 11:9 | ADC1 BIASCAL | ADC1 CALIB.BIASCOMP |
| 18:12 | OSC32K CAL | OSC32KCTRL.OSC32K.CALIB |
| 40:19 | CAL48M 5V | OSCCTRL.CAL48M[21:0] (VDD 3.6V–5.5V range) |
| 62:41 | CAL48M 3V3 | OSCCTRL.CAL48M[21:0] (VDD 2.7V–3.6V range) |
| 63 | Reserved | — |

Select the correct CAL48M entry based on the actual VDD supply voltage range.

## NVM Temperature Calibration Area — 0x00806030 (SAM C21 only)

Read-only. SAM C20 does NOT have this area. Used by TSENS peripheral.

| Bits | Name | Target Register |
|------|------|----------------|
| 5:0 | TSENS TCAL | TSENS CAL.TCAL |
| 11:6 | TSENS FCAL | TSENS CAL.FCAL |
| 35:12 | TSENS GAIN | TSENS GAIN |
| 59:36 | TSENS OFFSET | TSENS OFFSET |

## Device Serial Number — 128-bit Unique ID

Four 32-bit words at non-contiguous addresses. Uniqueness guaranteed only when all 128 bits are used.

| Word | Address |
|------|---------|
| Word 0 | 0x0080A00C |
| Word 1 | 0x0080A040 |
| Word 2 | 0x0080A044 |
| Word 3 | 0x0080A048 |

## Concepts Mentioned

- [[Memory Map]] — physical address space and NVM special areas
- [[NVMCTRL]] — writing User Row, BOOTPROT/LOCK/EEPROM settings
- [[SUPC]] — BODVDD configured via User Row at power-on
- [[WDT]] — WDT power-on settings from User Row
- [[OSCCTRL]] — CAL48M calibration from Software Calibration Area
- [[OSC32KCTRL]] — OSC32K calibration from Software Calibration Area
- [[ADC Configuration]] — ADC BIASREFBUF/BIASCOMP from Software Calibration Area
- [[TSENS Configuration]] — GAIN/OFFSET/CAL from Temperature Calibration Area

## See Also

- [[Memory Map]] — concept page with address space and calibration code examples
- [[NVMCTRL]] — Flash controller; read/write NVM User Row
- [[TSENS Configuration]] — how to load temperature calibration at init
- [[ADC Configuration]] — how to load ADC bias calibration at init
- [[OSCCTRL]] — how to load OSC48M calibration
