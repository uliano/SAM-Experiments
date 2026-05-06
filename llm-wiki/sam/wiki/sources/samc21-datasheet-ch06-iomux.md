---
title: SAMC21 Datasheet Chapter 6 — I/O Multiplexing and Considerations
type: source
tags: [port, pinmux, sercom, tc, tcc, eic, samc21, datasheet]
date: 2026-05-05
---

# SAMC21 Datasheet Ch. 6 — I/O Multiplexing and Considerations

Source: SAM C20/C21 Data Sheet DS60001479D, chapter 6, pages 27–35.

## Overview

Provides per-pin multiplexing tables for all package variants. Each pin supports up to 9 peripheral functions (A–I) selected via PMUX registers in PORT. This chapter also documents oscillator pin assignments, SWD pin locations, I²C-capable pins, and GPIO power supply clusters.

## Key Tables

| Table | Content |
|-------|---------|
| 6-1 | PORT Function Multiplexing for SAM C21 N (100-pin) |
| 6-2 | PORT Function Multiplexing for SAM C21 E/G/J (32/48/64-pin) — **use this for SAMC21J18A** |
| 6-3 | PORT Function Multiplexing for SAM C20 N (100-pin) |
| 6-4 | PORT Function Multiplexing for SAM C20 E/G/J |
| 6-5 | Oscillator Pinout (XOSC, XOSC32K) |
| 6-6 | SWD Interface Pinout |
| 6-7 | SERCOM Pins Supporting I²C (by package) |
| 6-8 | GPIO Clusters (power supply domain groupings) |
| 6-9 | TCC Configuration Summary |

## Key Facts

- **All analog functions are on peripheral function B.** Function B must be selected (PMUX=0x1) to disable digital control of the pin and avoid current leakage on floating analog inputs.
- SERCOM4 and SERCOM5 do not exist on SAM C21E (32-pin).
- Oscillator pins (PA14/PA15 for XOSC, PA00/PA01 for XOSC32K) are not PORT-controlled — leave PINCFG=0x00.
- SWD (PA30=SWCLK, PA31=SWDIO) auto-routes on debugger connection; avoid these pins for GPIO during debug.
- I²C requires specific pins (see Table 6-7); not all pins support open-drain output.

## TCC Summary

| TCC | CC | WO | Counter | Features |
|-----|----|----|---------|---------|
| TCC0 | 4 | 8 | 24-bit | Full feature set (fault, DTI, output matrix, pattern, dithering, SWAP) |
| TCC1 | 2 | 4 | 24-bit | Fault, dithering, pattern generation |
| TCC2 | 2 | 2 | 16-bit | Fault only |

## Power Domains (SAM C21)

- **VDDIO** (2.70–5.50V): I/O lines in VDDIO clusters + XOSC. Must be ≤ VDDIN.
- **VDDIN** (2.70–5.50V): I/O lines in VDDIN cluster (PA27-PA31, PB30-PB31) + OSC48M + internal regulator.
- **VDDANA** (2.70–5.50V): Analog I/O + ADC/AC/DAC/PTC/SDADC/32K oscillators. Same voltage as VDDIN.
- **VDDCORE** (~1.2V): Internal regulator output for CPU/peripherals/DPLL. Do not supply externally.

## See Also

- [[I/O Multiplexing]] — concept page with extracted SERCOM PAD table and board-specific pin summary
- [[PORT]] — register-level programming reference
- [[SERCOM UART Configuration]] — TXPO/RXPO and pad selection
