---
title: Timebase
type: entity
tags: [systick, timing, firmware]
sources: [firmware-core-library]
created: 2026-05-05
updated: 2026-05-05
---

# Timebase

SysTick-based millisecond/microsecond timebase. Defined in `src/core/timebase.hpp` /
`src/core/timebase.cpp`.

## API

```cpp
static void     init();
static uint32_t millis();
static uint32_t micros();
static void     on_systick_isr();   // must be called from SysTick_Handler
```

## Initialization

```cpp
Timebase::init();
```

Calls `SysTick_Config(F_CPU / 1000)` — configures SysTick to fire every 1 ms.
Sets `initialized_ = true` only if `SysTick_Config` returns 0 (success).
Both `millis()` and `micros()` return 0 if not initialized.

Constraint: `F_CPU / 1000` must fit in the 24-bit SysTick LOAD register (`F_CPU ≤ 16,777,216,000`
and `F_CPU ≥ 1000`). At 48 MHz: reload = 48000 — well within range.

## millis()

Returns milliseconds since `init()`. Protected by [[IRQ Critical Section]] (PRIMASK).
Wraps after ~49.7 days.

## micros()

Reads both `ms_count_` and `SysTick->VAL` in one critical section, then computes:

```cpp
elapsed_ticks = ticks_per_ms - val;
us = (elapsed_ticks * 1000) / ticks_per_ms;
return ms * 1000 + us;
```

Accounts for the case where the tick has elapsed but `on_systick_isr()` has not run yet —
checks `COUNTFLAG` and `SCB->ICSR.PENDSTSET`, adds 1 ms if pending.

## SysTick ISR Wiring

In `src/interrupts.cpp`:

```cpp
extern "C" void irq_handler_sys_tick(void) {
    Timebase::on_systick_isr();
}
```

The startup vector table points `SysTick_Handler` to `irq_handler_sys_tick`.

## Static Members

```cpp
static inline volatile uint32_t ms_count_ = 0;
static inline bool initialized_ = false;
```

`ms_count_` is `volatile` — read from both ISR and foreground code.

## Usage Pattern

```cpp
uint32_t start = Timebase::millis();
while ((Timebase::millis() - start) < 500) {
    __WFI();
}
```

Unsigned subtraction handles the 32-bit rollover correctly (wraps cleanly).

## See Also

- [[LineReader]] — uses `Timebase::millis()` for timeout tracking
- [[QuartzTest]] — uses millis for crystal startup timeout
- [[Clock System]] — F_CPU must be set correctly before `Timebase::init()`
