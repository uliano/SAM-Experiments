---
title: Package Variants
type: concept
tags: [package, pinout, gpio, samc21]
sources: [samc21-datasheet-ch04-pinout, samc21-datasheet-ch05-signal-descriptions]
created: 2026-05-05
updated: 2026-05-05
---

# Package Variants

The SAM C20/C21 family comes in four package sizes. The suffix letter in the
part number (e.g. ATSAMC21**J**18A) identifies the variant.

## Variant Summary

| Suffix | Pins | Package Types | Port A | Port B | Port C |
|--------|------|--------------|--------|--------|--------|
| E | 32 | VQFN-32, TQFP-32 | Subset | — | — |
| G | 48 | VQFN-48, TQFP-48 | PA00–PA31 | PB02–PB03, PB08–PB09 | — |
| J | 64 | VQFN-64, TQFP-64, WLCSP56 | PA00–PA31 | PB00–PB31 | — |
| N | 100 | TQFP-100 | PA00–PA31 | PB00–PB31 | PC00–PC28 |

## Key Facts

- The hardware target in this project is **ATSAMC21J18A-AU** (64-pin VQFN, 256 KB Flash).
- The E variant exposes only a subset of PA — consult the Ch.4 pinout diagram for the exact pin list.
- The G variant's PB is sparse: only PB02, PB03, PB08, PB09.
- The J WLCSP56 is a 56-ball BGA alternative for the J die; some J signals are absent.
- The N variant is the only one with Port C (PC00–PC28); it is TQFP-only (no VQFN at 100 pins).
- All variants provide: RESET (active-low), VDDIN (regulator in), VDDCORE (regulator out).
- Dedicated analog supply VDDANA / GNDANA present on G, J, N.

## Pin Naming Convention

Pins are named `P<port><number>` where port is A, B, or C and number is 00–31
(Port C stops at 28). Examples: PA14 (Port A pin 14), PB30, PC05.

## Peripheral Availability by Variant

Not all peripherals are reachable on all packages because the required pins may
be absent:

| Peripheral | Minimum Variant | Reason |
|------------|----------------|--------|
| SERCOM5 PB30/PB31 UART | J | PB30/PB31 absent on E and G |
| XOSC PA14/PA15 24 MHz | E, G, J, N | PA14/PA15 present everywhere |
| XOSC32K PA00/PA01 | E, G, J, N | PA00/PA01 present everywhere |
| SWD PA30/PA31 | E, G, J, N | always present |
| Full PTC X/Y coverage | J or N | requires full PA + PB |
| Port C peripherals | N only | PC exists only on N |
| CAN via PB12/PB13 | J or N | PB12/PB13 not in G variant |

## See Also

- [[SAMC21 Datasheet Ch.4 Pinout]] — full pin tables per package
- [[SAMC21 Datasheet Ch.5 Signal Descriptions]] — signal names per peripheral
- [[SAMC21 Datasheet Ch.6 I/O Multiplexing]] — PMUX letter assignments
- [[I/O Multiplexing]] — PMUX function letter table
- [[PORT]] — GPIO port register model
