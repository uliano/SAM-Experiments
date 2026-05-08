---
title: NVIC Interrupt Map
type: concept
tags: [nvic, interrupts, samc21, firmware]
sources: [samc21-datasheet-ch10-processor]
created: 2026-05-05
updated: 2026-05-05
---

# NVIC Interrupt Map

The SAM C21 NVIC has 32 lines (0–31) plus NMI. Multiple peripherals can share one line; the ISR must check INTFLAG registers to determine which source fired.

Priority levels: 4 (IPR0–IPR7). Priority 0 = highest.

## SAM C21 Interrupt Line Mapping

| NVIC Line | Peripheral(s) |
|-----------|--------------|
| NMI | EIC NMI |
| 0 | MCLK, OSCCTRL, OSC32KCTRL, SUPC, PAC |
| 1 | WDT |
| 2 | RTC |
| 3 | EIC |
| 4 | FREQM |
| 5 | TSENS |
| 6 | NVMCTRL |
| 7 | DMAC |
| 8 | EVSYS |
| 9 | SERCOM0 (+ SERCOM6 on N-series) |
| 10 | SERCOM1 (+ SERCOM7 on N-series) |
| 11 | SERCOM2 |
| 12 | SERCOM3 |
| 13 | SERCOM4 (G/J/N only) |
| 14 | SERCOM5 (G/J/N only) |
| 15 | CAN0 |
| 16 | CAN1 (G/J/N only) |
| 17 | TCC0 |
| 18 | TCC1 |
| 19 | TCC2 |
| 20 | TC0 (+ TC5 on N-series) |
| 21 | TC1 (+ TC6 on N-series) |
| 22 | TC2 (+ TC7 on N-series) |
| 23 | TC3 |
| 24 | TC4 |
| 25 | ADC0 |
| 26 | ADC1 |
| 27 | AC |
| 28 | DAC |
| 29 | SDADC |
| 30 | PTC |
| 31 | Reserved |

## SAM C20 Interrupt Line Mapping (differences from SAM C21)

| NVIC Line | SAM C21 | SAM C20 |
|-----------|---------|---------|
| 5 | TSENS | Reserved |
| 13 | SERCOM4 (G/J/N) | SERCOM4 (N only) |
| 14 | SERCOM5 (G/J/N) | SERCOM5 (N only) |
| 15 | CAN0 | Reserved |
| 16 | CAN1 (G/J/N) | Reserved |
| 26 | ADC1 | Reserved |
| 28 | DAC | Reserved |
| 29 | SDADC | Reserved |

SAM C20 has no TSENS, no CAN, no ADC1, no DAC, no SDADC on any variant.

## IRQn Values for CMSIS

The IRQn used in `NVIC_EnableIRQ(IRQn)` equals the NVIC line number above:

```cpp
NVIC_EnableIRQ(SERCOM5_IRQn);   // = 14
NVIC_SetPriority(SERCOM5_IRQn, 1);
```

## Multi-Peripheral Lines

Lines shared by multiple peripherals require the ISR to inspect each peripheral's INTFLAG to find the source. Line 0 is the most notable:

```cpp
void irq_handler_system(void) {
    // Check OSCCTRL->INTFLAG, MCLK->INTFLAG, etc.
}
```

The firmware `startup_samc21.hpp` maps IRQ handler names to vector table slots. SERCOM5 is on line 14.

## Startup SAMC21 Vector Table

The custom startup defines 31 peripheral IRQ slots (positions 0–30). Each slot corresponds to the NVIC line number. The vector table is placed at 0x00000000 and VTOR is set after `.data` and `.bss` initialization.

## CMSIS Register Addresses

| Register | Address |
|---------|---------|
| NVIC base | 0xE000E100 |
| ISER (enable set) | 0xE000E100 |
| ICER (enable clear) | 0xE000E180 |
| ISPR (pending set) | 0xE000E200 |
| ICPR (pending clear) | 0xE000E280 |
| IPR[n] (priority) | 0xE000E400 |

## See Also

- [[Startup SAMC21]] — vector table definition
- [[IRQ Critical Section]] — PRIMASK-based critical sections
- [[DMA Controller]] — uses NVIC line 7
- [[Uart INT]] / [[Uart DMA]] — use SERCOM5, line 14
