---
title: SAMC21 Datasheet Ch.4 Pinout
type: source
tags: [pinout, package, gpio, samc21, datasheet]
sources: [samc21-datasheet-ch04-pinout]
created: 2026-05-05
updated: 2026-05-05
---

# SAMC21 Datasheet Ch.4 Pinout

Package variants and pin assignments for the SAM C20/C21 family across all four
package sizes.

## Abstract

The SAM C20/C21 family is offered in four package sizes identified by a suffix
letter in the part number: E (32-pin), G (48-pin), J (64-pin, also WLCSP56),
and N (100-pin). Each larger variant exposes more GPIO and analog pins; only the
N variant adds a third I/O port (Port C). The chapter provides pinout diagrams
and tables for every variant.

## Key Facts

- Four package suffixes: E=32-pin, G=48-pin, J=64-pin (+WLCSP56 BGA), N=100-pin.
- SAM C21N/C20N (100-pin TQFP) is the only variant with Port C (PC00–PC28).
- All variants include Port A; the E has a subset of PA pins.
- G adds partial Port B (PB02–PB03, PB08–PB09); J and N have full PB (PB00–PB31).
- Power supply pins: VDDANA and GNDANA present on G, J, N (dedicated analog supply).
- VDDIN: input to internal voltage regulator; VDDCORE: regulator output for core.
- RESET pin is active-low; shared in all variants.
- WLCSP56 (56-ball BGA) is an alternative packaging of the J die with equivalent signals.

## Package Variants

| Suffix | Pins | Package Types | Port A | Port B | Port C |
|--------|------|--------------|--------|--------|--------|
| E | 32 | VQFN-32, TQFP-32 | Subset of PA | — | — |
| G | 48 | VQFN-48, TQFP-48 | PA00–PA31 | PB02–PB03, PB08–PB09 | — |
| J | 64 | VQFN-64, TQFP-64, WLCSP56 | PA00–PA31 | PB00–PB31 | — |
| N | 100 | TQFP-100 | PA00–PA31 | PB00–PB31 | PC00–PC28 |

## Notable Fixed-Function Pins (All Packages)

| Pin | Function | Notes |
|-----|----------|-------|
| PA14 | XIN | 24 MHz crystal input |
| PA15 | XOUT | 24 MHz crystal output |
| PA00 | XIN32 | 32.768 kHz crystal input |
| PA01 | XOUT32 | 32.768 kHz crystal output |
| PA30 | SWCLK | SWD clock (debug) |
| PA31 | SWDIO | SWD data (debug) |

## Entities Mentioned

- ATSAMC21E / ATSAMC20E (32-pin)
- ATSAMC21G / ATSAMC20G (48-pin)
- ATSAMC21J / ATSAMC20J (64-pin)
- ATSAMC21N / ATSAMC20N (100-pin)

## Concepts Mentioned

- [[Package Variants]] — detailed per-variant GPIO availability
- [[I/O Multiplexing]] — PMUX function assignment per pin
- [[PORT]] — GPIO register control for all ports

## Open Questions

- Exact subset of PA pins present on E variant (datasheet pinout diagram required).
- WLCSP56 ball map vs. VQFN-64 — which signals are absent in the BGA version.

## See Also

- [[SAMC21 Datasheet Ch.5 Signal Descriptions]] — signal names per peripheral
- [[SAMC21 Datasheet Ch.6 I/O Multiplexing]] — PMUX table
- [[Package Variants]] — concept page with pin availability details
- [[PORT]] — GPIO register model
- [[I/O Multiplexing]] — mux function table concept page
