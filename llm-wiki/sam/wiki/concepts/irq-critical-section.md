---
title: IRQ Critical Section
type: concept
tags: [firmware, interrupts, cortex-m0plus, concurrency]
sources: [firmware-core-library]
created: 2026-05-05
updated: 2026-05-05
---

# IRQ Critical Section

Pattern used throughout the firmware to protect shared state between ISR and foreground code.
Implemented via Cortex-M0+ PRIMASK register.

## Pattern

```cpp
static inline uint32_t irq_lock(void) {
    const uint32_t state = __get_PRIMASK();
    __disable_irq();
    return state;
}

static inline void irq_unlock(uint32_t state) {
    if (0 == (state & 1u))
        __enable_irq();
}

// Usage:
uint32_t irq_state = irq_lock();
// ... critical section ...
irq_unlock(irq_state);
```

## Why Save/Restore Rather Than Unconditionally Enable

Restoring the saved state rather than blindly calling `__enable_irq()` allows nested
critical sections. If interrupts were already disabled when `irq_lock()` was called (e.g.
called from within an ISR), they remain disabled after `irq_unlock()`. This prevents
accidental re-enabling of interrupts that were intentionally off.

## PRIMASK Bit 0

`PRIMASK.PM = 1` means all interrupts below NMI and HardFault are masked.
`__disable_irq()` sets bit 0; `__enable_irq()` clears it.
`__get_PRIMASK()` returns the full register; checking `state & 1` tells you if interrupts
were already disabled.

## Where It Is Used

- [[Uart INT]]: all `read()`, `write()`, `available()`, `get_stats()`, `tx_idle()` calls
- [[Uart DMA]]: same, plus `pump_rx_from_dma()`, `snapshot_rx_position()`
- [[Timebase]]: `millis()`, `micros()`

## Cortex-M0+ Note

The CM0+ has no BASEPRI register (unlike CM3/CM4), so PRIMASK is the only way to disable
interrupts selectively. All maskable interrupts are disabled — there is no per-priority
masking available on CM0+.

## See Also

- [[RingBuffer]] — not thread-safe on its own; relies on callers to use this pattern
- [[Uart INT]] / [[Uart DMA]] — concrete usage
- [[Timebase]] — millis/micros use it
