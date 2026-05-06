---
title: I/O Multiplexing
type: concept
tags: [port, pinmux, sercom, samc21, samc21j18a, io-mux]
sources: [samc21-datasheet-ch06-iomux]
created: 2026-05-05
updated: 2026-05-05
---

# I/O Multiplexing — SAM C21J18A (64-pin)

Each I/O pin can be assigned to a peripheral function by setting PINCFGy.PMUXEN=1 and writing PMUXn with the appropriate function letter (A–I). This page documents the function letter map and the pin assignments specific to the **SAMC21J18A-AU VQFN64/TQFP64** package.

## Function Letter Map

| PMUX[3:0] | Letter | Peripheral |
|-----------|--------|-----------|
| 0x0 | A | EIC (External Interrupt Controller) |
| 0x1 | B | Analog: ADC0/ADC1, AC, SDADC, PTC, DAC, REF |
| 0x2 | C | SERCOM (primary) |
| 0x3 | D | SERCOM-ALT (alternate) |
| 0x4 | E | TC / TCC |
| 0x5 | F | TCC |
| 0x6 | G | COM (CAN, AC/GCLK output) |
| 0x7 | H | AC/GCLK |
| 0x8 | I | CCL (Configurable Custom Logic) |

**Note:** Function B must be selected for analog pins — this disables the digital input buffer automatically.

## Special-Function Pins (Not PORT-Muxed)

### Oscillator Pins (Table 6-5)

Controlled by OSCCTRL/OSC32KCTRL, not by PORT PMUXEN:

| Oscillator | Signal | Pin |
|-----------|--------|-----|
| XOSC | XIN | PA14 |
| XOSC | XOUT | PA15 |
| XOSC32K | XIN32 | PA00 |
| XOSC32K | XOUT32 | PA01 |

These pins must be left at reset state in PORT (PINCFG=0x00, INEN=0).

### SWD Pins (Table 6-6)

| Signal | Pin | Supply |
|--------|-----|--------|
| SWCLK | PA30 | VDDIN |
| SWDIO | PA31 | VDDIN |

SWD pins share PORT function — SWDIO is auto-routed by debugger detection.

## SERCOM PAD Assignments — SAM C21J (64-pin)

Each SERCOM has up to 4 I/O pads. PAD assignments depend on the function letter (C = primary, D = alternate). For UART, TX=PAD[0], RX=PAD[1] or PAD[3] (see [[SERCOM UART Configuration]] for TXPO/RXPO details).

| SERCOM | Function | PAD[0] | PAD[1] | PAD[2] | PAD[3] |
|--------|---------|--------|--------|--------|--------|
| SERCOM0 | C | PA08 | PA09 | PA10 | PA11 |
| SERCOM0 | D | PA04 | PA05 | PA06 | PA07 |
| SERCOM1 | C | PA16 | PA17 | PA18 | PA19 |
| SERCOM1 | D | PA00 | PA01 | PA30 | PA31 |
| SERCOM2 | C | PA12 | PA13 | PA14 | PA15 |
| SERCOM2 | D | PA08 | PA09 | PA10 | PA11 |
| SERCOM3 | C | PA22 | PA23 | PA24 | PA25 |
| SERCOM3 | D | PA16 | PA17 | PA18 | PA19 |
| SERCOM4 | C | PB12 | PB13 | PB14 | PB15 |
| SERCOM4 | D | PA12 | PA13 | PA14 | PA15 |
| SERCOM5 | C | PA22 | PA23 | PA20 | PA21 |
| SERCOM5 | D | **PB30** | **PB31** | PB00 | PB01 |

**This board uses SERCOM5 function D: PB30=PAD[0] (TX), PB31=PAD[1] (RX).**

Note: SERCOM4 and SERCOM5 do not exist on SAM C21E (32-pin). On SAM C21G (48-pin), SERCOM4 is supported but SERCOM5 is limited.

## SERCOM I²C Capable Pins (64-pin)

Only certain pins support I²C (open-drain output). For SAMC21J/64-pin:

```
PA08, PA09, PA12, PA13, PA16, PA17, PA22, PA23
PB12, PB13, PB16, PB17, PB30, PB31
```

## TC/TCC Output Pin Examples

| Pin | Function E | Function F |
|-----|-----------|-----------|
| PA08 | TC0/WO[0] | TCC0/WO[0] |
| PA09 | TC0/WO[1] | TCC0/WO[1] |
| PA10 | TC1/WO[0] | TCC1/WO[0] |
| PA11 | TC1/WO[1] | TCC1/WO[1] |
| PA22 | TC3/WO[0] | TCC0/WO[4] |
| PA23 | TC3/WO[1] | TCC0/WO[5] |
| PB30 | TCC0/WO[0] | TCC1/WO[2] |
| PB31 | TCC0/WO[1] | TCC1/WO[3] |

## TCC Configuration Summary (Table 6-9)

| TCC | CC channels | WO outputs | Counter | Features |
|-----|------------|-----------|---------|---------|
| TCC0 | 4 | 8 | 24-bit | Fault, Dithering, Output matrix, Dead-time, SWAP, Pattern gen |
| TCC1 | 2 | 4 | 24-bit | Fault, Dithering, Pattern gen |
| TCC2 | 2 | 2 | 16-bit | Fault |

## Board Pin Summary (SAMC21J18A-AU)

| Pin | Function | Notes |
|-----|---------|-------|
| PA00 | XOSC32K XIN32 | Oscillator, analog — leave at reset if unpopulated |
| PA01 | XOSC32K XOUT32 | Oscillator, analog |
| PA03 | VREF | Capacitively loaded analog reference — leave at reset |
| PA04 | VREF | Capacitively loaded analog reference — leave at reset |
| PA14 | XOSC XIN | 24 MHz crystal — leave at reset state |
| PA15 | XOSC XOUT | 24 MHz crystal — leave at reset state |
| PA30 | SWCLK | SWD (shared with SERCOM1-D PAD[2]) |
| PA31 | SWDIO | SWD (shared with SERCOM1-D PAD[3]) |
| PB22 | Bench button | GPIO input, INEN=1, external pull-down |
| PB23 | Bench LED | GPIO output, active high |
| PB30 | SERCOM5 TX | PAD[0], function D, PMUXEN=1 |
| PB31 | SERCOM5 RX | PAD[1], function D, PMUXEN=1, INEN=1 |

## See Also

- [[PORT]] — registers for configuring PMUXEN and PINCFG per pin
- [[SERCOM UART Configuration]] — TXPO/RXPO PAD mapping and baud formula
- [[Pin]] — firmware `Pin<Bank,N>` GPIO wrapper
- [[OSCCTRL]] — PA14/PA15 XOSC configuration (not via PORT)
- [[OSC32KCTRL]] — PA00/PA01 XOSC32K configuration (not via PORT)
- [[SAMC21 Datasheet Ch. 6 — I/O Multiplexing]] — source with full Table 6-2 per-pin mux table
