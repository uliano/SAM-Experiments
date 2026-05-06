---
title: SAMC21 Datasheet Ch.5 Signal Descriptions
type: source
tags: [signal, pinout, gpio, peripheral, samc21, datasheet]
sources: [samc21-datasheet-ch05-signal-descriptions]
created: 2026-05-05
updated: 2026-05-05
---

# SAMC21 Datasheet Ch.5 Signal Descriptions

Complete mapping of peripheral signal names to port pins for the SAM C20/C21.

## Abstract

Chapter 5 provides a signal-centric view of the device: for each peripheral it
lists the signal names exposed on external pins and the physical ports that carry
them. This complements the PMUX tables in Chapter 6 by naming every signal before
any mux letter is assigned. Use this chapter to determine which peripherals a
given pin can serve and to cross-reference pin-count requirements when selecting
a package variant.

## Key Facts

- All SERCOM instances expose PAD[3:0]; the function (USART/SPI/I2C) determines
  which PAD carries TX/RX/SCK/SDA/SCL etc.
- AC has eight analog input pins AIN[7:0] and three output signals CMP[2:0].
- ADC has 20 single-ended channels AIN[19:0] and an external reference VREFA.
- SDADC has three differential input pairs (AINP/AINN[2:0]) and reference VREFB.
- DAC has one output VOUT and shares VREFA with ADC (SAM C21 only).
- EIC provides 16 external interrupt lines EXTINT[15:0] plus NMI.
- GCLK exposes 8 generic clock I/O pins GCLK_IO[7:0].
- CCL (Configurable Custom Logic) has 12 inputs IN[11:0] and 4 outputs OUT[3:0].
- PTC (touch controller) has 16 drive lines X[15:0] and 16 sense lines Y[15:0].
- CAN exposes TX and RX.
- Oscillator: XIN/XOUT (main crystal), XIN32/XOUT32 (32 kHz crystal).
- SWD debug: SWCLK (PA30), SWDIO (PA31) — shared with GPIO / SERCOM5 alternate.

## Signal Summary Table

| Peripheral | Signal Names | Count |
|------------|-------------|-------|
| AC | AIN[7:0], CMP[2:0] | 11 |
| ADC | AIN[19:0], VREFA | 21 |
| DAC | VOUT, VREFA | 2 |
| SDADC | AINP[2:0], AINN[2:0], VREFB | 7 |
| EIC | EXTINT[15:0], NMI | 17 |
| GCLK | GCLK_IO[7:0] | 8 |
| CCL | IN[11:0], OUT[3:0] | 16 |
| SERCOM (each) | PAD[3:0] | 4 |
| OSCCTRL | XIN, XOUT | 2 |
| OSC32KCTRL | XIN32, XOUT32 | 2 |
| TC (each) | WO[1:0] | 2 |
| TCC | WO[7:0] | up to 8 |
| PTC | X[15:0], Y[15:0] | 32 |
| CAN | TX, RX | 2 |
| PORT | PA[31:0], PB[31:0], PC[28:0] | up to 92 |
| PM/RSTC | RESET | 1 |

## Concepts Mentioned

- [[I/O Multiplexing]] — PMUX assignment of signals to physical pins
- [[Package Variants]] — signal availability depends on package variant
- [[PORT]] — GPIO port model (PA/PB/PC)

## Open Questions

- Exact port/pin assignments for each signal (in Ch.6 PMUX tables, not Ch.5).
- Which SERCOM PADs are present on the 32-pin E variant.

## See Also

- [[SAMC21 Datasheet Ch.4 Pinout]] — physical pin assignments by package
- [[SAMC21 Datasheet Ch.6 I/O Multiplexing]] — PMUX table mapping signals to pins
- [[Package Variants]] — which pins are available on which package
- [[I/O Multiplexing]] — PMUX function letter table concept page
- [[PORT]] — GPIO register model
