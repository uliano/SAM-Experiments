---
title: PORT
type: concept
tags: [port, gpio, pinmux, pincfg, samc21]
sources: [samc21-datasheet-ch28-port]
created: 2026-05-05
updated: 2026-05-05
---

# PORT — I/O Pin Controller

PORT controls all I/O pads on the SAM C21. Pins are organised in groups of up to 32 (Group 0 = PA, Group 1 = PB). Registers are duplicated per group with a **0x80 byte offset** between groups.

Base addresses:
- Group A (PA): **0x41008000** (APB-B)
- Group B (PB): **0x41008080**

APB clock is in APBBMASK bit 0 (PORT), reset=1 — always enabled.

## Register Map (per group, offset from group base)

| Offset | Name | Access | Description |
|--------|------|--------|-------------|
| 0x00 | DIR | RW | Data Direction (1=output) |
| 0x04 | DIRCLR | W | Direction Clear (write 1 → input, no RMW needed) |
| 0x08 | DIRSET | W | Direction Set (write 1 → output) |
| 0x0C | DIRTGL | W | Direction Toggle |
| 0x10 | OUT | RW | Data Output Value (or pull direction when input+pull) |
| 0x14 | OUTCLR | W | Output Clear (drive low / pull-down) |
| 0x18 | OUTSET | W | Output Set (drive high / pull-up) |
| 0x1C | OUTTGL | W | Output Toggle |
| 0x20 | IN | R | Data Input Value |
| 0x24 | CTRL | RW | Input Sampling Mode (SAMPLING[31:0]) |
| 0x28 | WRCONFIG | W | Write Configuration (batch multi-pin setup, write-only) |
| 0x2C | EVCTRL | RW | Event Input Control (4 event inputs per group) |
| 0x30–0x3F | PMUX0–PMUX15 | RW | Peripheral Multiplexing (one byte per two pins) |
| 0x40–0x5F | PINCFG0–PINCFG31 | RW | Pin Configuration (one byte per pin) |

All DIR/OUT/IN registers are 32-bit; PMUX and PINCFG registers are 8-bit.

## Reset State

After reset: **all pins are inputs with input buffers disabled and outputs tri-stated** (DIR=0, INEN=0, PULLEN=0). This is also the correct state for analog pins.

## Pin Configuration Truth Table

| DIR | INEN | PULLEN | OUT | Configuration |
|-----|------|--------|-----|---------------|
| 0 | 0 | 0 | X | Reset / analog — all digital disabled |
| 0 | 0 | 1 | 0 | Pull-down; input buffer disabled |
| 0 | 0 | 1 | 1 | Pull-up; input buffer disabled |
| 0 | 1 | 0 | X | Standard input (floating) |
| 0 | 1 | 1 | 0 | Input with pull-down |
| 0 | 1 | 1 | 1 | Input with pull-up |
| 1 | 0 | X | X | Output; input buffer disabled |
| 1 | 1 | X | X | Output with read-back enabled |

When pull is enabled on an input: OUT=0 → pull-down, OUT=1 → pull-up.
Enabling output driver automatically disables pull.

## PINCFG Register (offset 0x40+n, 8-bit, one per pin)

| Bit | Name | Description |
|-----|------|-------------|
| 6 | DRVSTR | Output driver strength: 0=normal, 1=stronger |
| 2 | PULLEN | Pull enable (direction set by OUT bit) |
| 1 | INEN | Input buffer enable (must be 1 to read IN register) |
| 0 | PMUXEN | Peripheral multiplexer enable (overrides PORT direction/output) |

When PMUXEN=1, the peripheral selected in PMUXn controls the pad; PORT DIR/OUT are ignored. The IN register can still be read if INEN=1.

## PMUXn Register (offset 0x30+n, 8-bit, one per two pins)

Each byte covers **two consecutive pins**: bits 7:4 = PMUXO (odd pin 2n+1), bits 3:0 = PMUXE (even pin 2n).

| PMUX[3:0] | Function | | PMUX[3:0] | Function |
|-----------|----------|-|-----------|----------|
| 0x0 | A | | 0x5 | F |
| 0x1 | B | | 0x6 | G |
| 0x2 | C | | 0x7 | H |
| 0x3 | D | | 0x8 | I |
| 0x4 | E | | 0x9–0xF | Reserved |

See [[I/O Multiplexing]] for which function letter maps to which peripheral on each pin.

### Enabling a Peripheral on a Pin

```cpp
// Example: enable SERCOM5 PAD[0] on PB30 (function D)
// PB30 = group B, pin 30. PMUXn index = 30/2 = 15, O (odd)
// PINCFG offset = 0x40 + 30 = 0x5E

// Set PMUX function D (SERCOM alt) for pin 30 (odd → PMUXO)
PORT->Group[1].PMUX[15].reg =
    (PORT->Group[1].PMUX[15].reg & ~PORT_PMUX_PMUXO_Msk) |
    PORT_PMUX_PMUXO(3);   // 0x3 = D

// Enable peripheral mux on pin 30 (TX — no INEN needed)
PORT->Group[1].PINCFG[30].reg = PORT_PINCFG_PMUXEN;

// PB31 (RX, pin 31, even → PMUXE)
PORT->Group[1].PMUX[15].reg =
    (PORT->Group[1].PMUX[15].reg & ~PORT_PMUX_PMUXE_Msk) |
    PORT_PMUX_PMUXE(3);   // 0x3 = D

PORT->Group[1].PINCFG[31].reg = PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN;
```

Note: pins 30 and 31 share PMUX[15] — configure both before writing if possible, or use read-modify-write.

## WRCONFIG — Batch Pin Configuration (offset 0x28, 32-bit, write-only)

Configures multiple pins in a single 32-bit write. Read always returns 0.

| Bit | Name | Description |
|-----|------|-------------|
| 31 | HWSEL | 0=lower 16 pins (0–15), 1=upper 16 pins (16–31) |
| 30 | WRPINCFG | 1=update PINCFGy for selected pins |
| 28 | WRPMUX | 1=update PMUXn for selected pins |
| 27:24 | PMUX[3:0] | PMUX value to write to selected pins |
| 22 | DRVSTR | Value to write to PINCFGy.DRVSTR |
| 18 | PULLEN | Value to write to PINCFGy.PULLEN |
| 17 | INEN | Value to write to PINCFGy.INEN |
| 16 | PMUXEN | Value to write to PINCFGy.PMUXEN |
| 15:0 | PINMASK | Bit mask of pins to configure (within the selected half-word) |

Must be written as a single 32-bit write — 8-bit or 16-bit writes have no effect.

## CTRL — Input Sampling (offset 0x24)

CTRL.SAMPLING[31:0]: 0=on-demand (2-cycle latency when reading IN), 1=continuous sampling.

Use continuous sampling when reading pins via the ARM IOBUS (single-cycle I/O port). Without it, the IN register may return stale data.

The existing `Pin<Bank,N>` wrapper in the firmware reads via IOBUS — ensure SAMPLING is set if fast GPIO reads are needed. See [[Pin]].

## GPIO Patterns

```cpp
// Output: set PB23 high (bench LED, active high)
PORT->Group[1].DIRSET.reg = PORT_PA23;   // note: PB23 in group 1
PORT->Group[1].OUTSET.reg = PORT_PA23;

// Input: read PB22 (bench button, active high, external pull-down)
PORT->Group[1].DIRCLR.reg = PORT_PA22;
PORT->Group[1].PINCFG[22].reg = PORT_PINCFG_INEN;
bool pressed = (PORT->Group[1].IN.reg & PORT_PA22) != 0;

// Analog: leave PINCFG=0x00 (reset default) — no INEN, no PMUXEN
// PA03/PA04 on this board are VREF pins — leave at reset state
```

## Board Pin Assignments (SAMC21J18A-AU)

Key pins from CLAUDE.md:

| Pin | Group | Signal | Config |
|-----|-------|--------|--------|
| PA14 | A | XOSC XIN | Analog (PMUXEN=0, INEN=0) |
| PA15 | A | XOSC XOUT | Analog |
| PA00 | A | XOSC32K XIN32 | Analog (if populated) |
| PA01 | A | XOSC32K XOUT32 | Analog (if populated) |
| PB30 | B | SERCOM5 TX (PAD[0]) | PMUXEN=1, func D |
| PB31 | B | SERCOM5 RX (PAD[1]) | PMUXEN=1, INEN=1, func D |
| PB23 | B | Bench LED | Output |
| PB22 | B | Bench button | Input, INEN=1 |
| PA03 | A | VREF | Analog (leave at reset) |
| PA04 | A | VREF | Analog (leave at reset) |

## See Also

- [[I/O Multiplexing]] — full peripheral function letter tables for SAMC21J18A (Ch. 6)
- [[Pin]] — firmware `Pin<Bank,N>` wrapper using DIRSET/OUTSET/OUTCLR/IN
- [[SERCOM UART Configuration]] — PMUX function D/C for SERCOM pads
- [[SAMC21 Datasheet Ch. 28 — PORT]] — register-level source reference
- [[Memory Map]] — PORT base address and APB-B location
