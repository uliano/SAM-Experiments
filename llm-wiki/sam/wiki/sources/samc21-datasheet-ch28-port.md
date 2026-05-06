---
title: SAMC21 Datasheet Chapter 28 — PORT
type: source
tags: [port, gpio, pinmux, pincfg, samc21, datasheet]
date: 2026-05-05
---

# SAMC21 Datasheet Ch. 28 — PORT I/O Pin Controller

Source: SAM C20/C21 Data Sheet DS60001479D, chapter 28, pages 435–462.

## Overview

PORT controls all I/O pads, organised as port groups (A, B, ...) with 0x80 address spacing. Group A (PA) base = 0x41008000. Supports GPIO, peripheral mux (A–I), pull resistors, drive strength, and event-driven pin control.

## Register Summary (per group)

| Offset | Name | Access | Description |
|--------|------|--------|-------------|
| 0x00 | DIR | RW | Data direction (1=output) |
| 0x04/08/0C | DIRCLR/DIRSET/DIRTGL | W | Atomic direction change (no RMW) |
| 0x10 | OUT | RW | Output value / pull direction |
| 0x14/18/1C | OUTCLR/OUTSET/OUTTGL | W | Atomic output change |
| 0x20 | IN | R | Sampled input value |
| 0x24 | CTRL | RW | SAMPLING[31:0] — continuous vs on-demand input |
| 0x28 | WRCONFIG | W | Batch config for up to 16 pins in one 32-bit write |
| 0x2C | EVCTRL | RW | Up to 4 event inputs per group; action OUT/SET/CLR/TGL |
| 0x30–0x3F | PMUX0–15 | RW | Peripheral mux: PMUXO[3:0] (odd pin) + PMUXE[3:0] (even pin) |
| 0x40–0x5F | PINCFG0–31 | RW | Per-pin: DRVSTR(6), PULLEN(2), INEN(1), PMUXEN(0) |

## Key Facts

- Reset state: all pins input, input buffers disabled, tri-stated — correct for analog pins too.
- To use as GPIO output: write DIR bit; use OUTSET/OUTCLR/OUTTGL for atomic operations.
- To use as GPIO input: clear DIR, set PINCFGy.INEN=1; optionally set PULLEN+OUT for pull.
- To use as peripheral: set PINCFGy.PMUXEN=1 AND write PMUXn with function code (0x0=A … 0x8=I).
- PMUXn covers two pins per byte: bits 7:4 = PMUXO (odd pin 2n+1), bits 3:0 = PMUXE (even pin 2n).
- WRCONFIG is write-only, 32-bit only; useful to configure multiple pins at once.
- CTRL.SAMPLING must be set for pins read via ARM IOBUS to avoid stale IN data.

## See Also

- [[PORT]] — concept page with full register details, truth table, code examples, board pin table
- [[I/O Multiplexing]] — Ch. 6, peripheral function letter assignments per pin
- [[Pin]] — firmware GPIO wrapper
- [[SERCOM UART Configuration]] — peripheral mux usage example
