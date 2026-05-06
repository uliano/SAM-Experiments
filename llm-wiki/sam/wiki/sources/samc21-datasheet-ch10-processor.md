---
title: SAMC21 Datasheet Ch.10 Processor and Architecture
type: source
tags: [cortex-m0, nvic, interrupt, mpu, mtb, samc21, datasheet]
sources: [samc21-datasheet-ch10-processor]
created: 2026-05-05
updated: 2026-05-05
---

# SAMC21 Datasheet Ch.10 Processor and Architecture

Cortex-M0+ processor configuration, NVIC interrupt mapping, Micro Trace Buffer,
and high-speed bus system for the SAM C20/C21.

## Abstract

The SAM C20/C21 implements the ARM Cortex-M0+ (revision r0p1) based on ARMv6
Thumb-2 ISA. It provides 32 external interrupt lines, 4 priority levels, an
8-region MPU, and SysTick. A key architectural detail is the single-cycle I/O
port bus that provides 1-cycle access to PORT and DIVAS registers independently
from the AHB-Lite system bus. The chapter includes full NVIC line mapping tables
for both SAM C21 and SAM C20.

## Key Facts

- Core: ARM Cortex-M0+, revision r0p1, ARMv6 Thumb-2 ISA.
- 32 external interrupt lines (IRQ0–IRQ31) plus NMI.
- 4 priority levels (0 = highest, set via IPR0–IPR7 registers).
- MPU: 8-region Memory Protection Unit.
- SysTick: 24-bit timer, clocked by CLK_CPU.
- Multiplier: fast, single cycle.
- Wake-up interrupt controller (WIC): **NOT present** on SAM C20/C21.
- VTOR (Vector Table Offset Register): present — VTOR at 0xE000ED08.
- Unprivileged/privileged thread modes: supported.
- Reset all registers: **NOT supported** (registers hold values across software reset).
- Instruction fetch width: 32-bit.
- Two bus interfaces:
  - AHB-Lite system bus (flash, SRAM, peripherals via APB bridges).
  - Single-cycle I/O port bus (PORT and DIVAS only, 1-cycle load/store).

## Cortex-M0+ Configuration (SAM C20/C21 Specific)

| Feature | SAM C20/C21 Setting |
|---------|-------------------|
| Interrupts | 32 (IRQ0–IRQ31) |
| Data endianness | Little-endian |
| SysTick | Present |
| Watchpoint comparators | 2 |
| Breakpoint comparators | 4 |
| Halting debug | Present |
| Multiplier | Fast (single cycle) |
| Single-cycle I/O port | Present (PORT + DIVAS) |
| Wake-up interrupt controller | Not supported |
| VTOR | Present |
| Unprivileged/Privileged | Present |
| MPU | 8-region |
| Reset all registers | Absent |
| Instruction fetch width | 32-bit |

## Cortex-M0+ Address Map

| Address | Peripheral |
|---------|-----------|
| 0xE000E000 | System Control Space (SCS) |
| 0xE000E010 | System Timer (SysTick) |
| 0xE000E100 | Nested Vector Interrupt Controller (NVIC) |
| 0xE000ED00 | System Control Block (SCB) |
| 0x41008000 | Micro Trace Buffer (MTB) |

## NVIC Interrupt Line Mapping — SAM C21

| NVIC Line | Peripheral(s) |
|-----------|--------------|
| NMI | EIC NMI |
| 0 | PM, MCLK, OSCCTRL, OSC32KCTRL, SUPC, PAC (all shared) |
| 1 | WDT |
| 2 | RTC |
| 3 | EIC |
| 4 | FREQM |
| 5 | TSENS |
| 6 | NVMCTRL |
| 7 | DMAC |
| 8 | EVSYS |
| 9 | SERCOM0 (+ SERCOM6 on N-series only) |
| 10 | SERCOM1 (+ SERCOM7 on N-series only) |
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

## NVIC Interrupt Line Mapping — SAM C20

Differences from SAM C21: no TSENS (line 5 = Reserved), no CAN1 on any variant
(line 16 = Reserved), no SERCOM4/5 on E variant, no ADC1 (line 26 = Reserved),
no DAC (line 28 = Reserved), no SDADC (line 29 = Reserved).

| NVIC Line | Peripheral(s) |
|-----------|--------------|
| NMI | EIC NMI |
| 0 | PM, MCLK, OSCCTRL, OSC32KCTRL, SUPC, PAC |
| 1 | WDT |
| 2 | RTC |
| 3 | EIC |
| 4 | FREQM |
| 5 | Reserved |
| 6 | NVMCTRL |
| 7 | DMAC |
| 8 | EVSYS |
| 9 | SERCOM0 (+ SERCOM6 on N) |
| 10 | SERCOM1 (+ SERCOM7 on N) |
| 11 | SERCOM2 |
| 12 | SERCOM3 |
| 13 | SERCOM4 (N only) |
| 14 | SERCOM5 (N only) |
| 15 | Reserved |
| 16 | Reserved |
| 17 | TCC0 |
| 18 | TCC1 |
| 19 | TCC2 |
| 20 | TC0 (+ TC5 on N) |
| 21 | TC1 (+ TC6 on N) |
| 22 | TC2 (+ TC7 on N) |
| 23 | TC3 |
| 24 | TC4 |
| 25 | ADC0 |
| 26 | Reserved |
| 27 | AC |
| 28 | Reserved |
| 29 | Reserved |
| 30 | PTC |
| 31 | Reserved |

## Micro Trace Buffer (MTB)

- Base address: 0x41008000 (APB-B).
- CoreSight MTB-M0+; stores trace packets in SRAM.
- Trace packet = pair of 32-bit words recording non-sequential PC changes.
- Registers: POSITION (write pointer + wrap bit), MASTER (enable/watermark), FLOW (AUTOSTOP/AUTOHALT), BASE (SRAM location for auto-discovery).
- Tracing starts when MASTER.EN=1; stops when watermark reached (AUTOSTOP/AUTOHALT).
- MTB SRAM access is shared with the processor; trace writes take priority.

## High-Speed Bus System

- Symmetric crossbar bus matrix; concurrent access by different masters to different slaves.
- 32-bit data bus.
- Operates at 1:1 CPU clock frequency.
- Masters: Cortex-M0+ system bus, I/O port bus, DMAC, DSU.
- Single-cycle I/O access: PORT and DIVAS registers only (via I/O port bus, not AHB).

## Concepts Mentioned

- [[NVIC Interrupt Map]] — full NVIC table for SAM C21
- [[Memory Map]] — peripheral base addresses, Cortex-M0+ register addresses
- [[IRQ Critical Section]] — PRIMASK-based critical sections on CM0+
- [[DMA Controller]] — bus master on the high-speed crossbar
- [[Startup SAMC21]] — vector table, VTOR setup

## See Also

- [[NVIC Interrupt Map]] — concept page with CMSIS usage examples
- [[IRQ Critical Section]] — PRIMASK/critical section patterns
- [[Memory Map]] — full peripheral address map
- [[Startup SAMC21]] — vector table layout, VTOR initialization
- [[DMA Controller]] — DMAC bus master architecture
