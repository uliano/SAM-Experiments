---
title: Startup SAMC21
type: entity
tags: [firmware, startup, vector-table, cortex-m0plus]
sources: [firmware-core-library]
created: 2026-05-05
updated: 2026-05-05
---

# Startup SAMC21

Custom vector table and reset handler for the SAMC21. Defined in `src/core/startup_samc21.c`.
Based on Alex Taradov's minimal SAMC21 startup template.

## Vector Table

Placed in `.vectors` section (linker script maps this to flash start):

```c
__attribute__ ((used, section(".vectors")))
void (* const vectors[])(void) = {
    &_stack_top,            // 0: initial SP
    irq_handler_reset,      // 1: Reset
    irq_handler_nmi,        // 2: NMI
    irq_handler_hard_fault, // 3: Hard Fault
    // 4–10: reserved (0)
    irq_handler_sv_call,    // 11: SVCall
    // 12–13: reserved
    irq_handler_pend_sv,    // 14: PendSV
    irq_handler_sys_tick,   // 15: SysTick
    // Peripheral IRQs 0–30: see table below
};
```

## Peripheral IRQ Map

| Index | Handler | Peripheral |
|-------|---------|------------|
| 0 | `irq_handler_system` | System Controller (OSCCTRL, MCLK, ...) |
| 1 | `irq_handler_wdt` | Watchdog Timer |
| 2 | `irq_handler_rtc` | Real Time Counter |
| 3 | `irq_handler_eic` | External Interrupt Controller |
| 4 | `irq_handler_freqm` | Frequency Meter |
| 5 | `irq_handler_tsens` | Temperature Sensor |
| 6 | `irq_handler_nvmctrl` | NVM Controller |
| 7 | `irq_handler_dmac` | DMA Controller |
| 8 | `irq_handler_evsys` | Event System |
| 9–14 | `irq_handler_sercom0`–`5` | SERCOM 0–5 |
| 15–16 | `irq_handler_can0`–`1` | CAN 0–1 |
| 17–19 | `irq_handler_tcc0`–`2` | TCC 0–2 |
| 20–24 | `irq_handler_tc0`–`4` | TC 0–4 |
| 25–26 | `irq_handler_adc0`–`1` | ADC 0–1 |
| 27 | `irq_handler_ac` | Analog Comparator |
| 28 | `irq_handler_dac` | DAC |
| 29 | `irq_handler_sdadc` | Sigma-Delta ADC |
| 30 | `irq_handler_ptc` | Peripheral Touch Controller |

## Default Handler

All handlers not overridden are `DUMMY` (weak alias to `irq_handler_dummy`):

```c
void irq_handler_dummy(void) { while (1); }
```

An unexpected interrupt hangs the CPU — useful for debugging: a runaway IRQ will
lock up visibly rather than silently corrupting state.

## Reset Handler

```c
void irq_handler_reset(void) {
    copy .data from _etext to RAM;   // flash → RAM for initialized globals
    zero .bss;                        // uninitalized globals → 0
    __libc_init_array();              // C++ constructors, newlib init
    SCB->VTOR = vectors;             // set VTOR (not set by hardware on CM0+)
    main();
    while(1);                         // trap if main() returns
}
```

**VTOR must be set explicitly** — Cortex-M0+ does not automatically use the link-time
vector table address after reset; it defaults to address 0x0.

## Wiring ISRs

To implement a peripheral handler, define it with the matching name:

```cpp
// In interrupts.cpp:
extern "C" void irq_handler_sercom5(void) {
    Serial.irq_handler();
}
extern "C" void irq_handler_sys_tick(void) {
    Timebase::on_systick_isr();
}
extern "C" void irq_handler_dmac(void) {
    // forward to UartDMA instance if using DMA UART
}
```

The linker resolves the strong definition over the weak alias automatically.

## See Also

- [[Syscalls]] — newlib bridge (requires Serial to be ready before `_write`)
- [[Timebase]] — SysTick ISR wiring
- [[Uart INT]] — SERCOM ISR wiring
- [[Uart DMA]] — DMAC ISR wiring
