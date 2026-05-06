---
title: Firmware Core Library
type: source
tags: [firmware, c++, bare-metal, samc21]
sources: [firmware-core-library]
created: 2026-05-05
updated: 2026-05-05
---

# Firmware Core Library

Reusable bare-metal C++23 utility library for SAMC21 projects. All files live in `src/core/`.
No Arduino, no ASF, no HAL — direct register access throughout.

## Files

| File | Description |
|------|-------------|
| `pin.hpp` | Zero-cost `Pin<Bank,N>` GPIO wrapper |
| `ring_buffer.hpp` | Power-of-2 `RingBuffer<T,N>` with overwrite semantics |
| `bytestream.hpp` | `ByteStream` C++20 concept + `ByteStreamPrintMixin<T>` CRTP |
| `serial_port.hpp` | `SerialPort<UartT>` facade combining ByteStream + Uart |
| `uart.hpp` | Uart dispatcher: `SercomTraits<N>`, `UartPinout<...>`, mode aliases |
| `uart_int.hpp` | `Uart<UartMode::Interrupt, ...>` — ISR-driven TX/RX ring buffers |
| `uart_dma.hpp` | `Uart<UartMode::Dma, ...>` — DMAC-driven TX/RX |
| `timebase.hpp/.cpp` | `Timebase` — SysTick-based millis/micros |
| `line_reader.hpp` | `LineReader<StreamT>` — non-blocking line input with echo/timeout |
| `startup_samc21.c` | Vector table, reset handler, dummy ISR stubs |
| `syscalls.cpp` | newlib `_write`/`_read` bridged to `Serial` |
| `include/` | CMSIS CM0+ headers + SAMC21 device register headers |

## Key Design Decisions

- All GPIO via compile-time template parameters → zero runtime overhead, direct register access.
- `RingBuffer` uses power-of-2 size (N = exponent) so masking is `& (size-1)` — no modulo.
- `RingBuffer::push` overwrites oldest entry when full instead of dropping — callers must check return value if loss matters.
- `ByteStream` is a C++20 concept, not a virtual base — no vtable, no dynamic dispatch.
- UART TX is non-blocking: drops bytes when buffer full, reports via `Stats`.
- IRQ critical sections use PRIMASK (`__get_PRIMASK`/`__disable_irq`/`__enable_irq`) — saves and restores state.
- `std::to_chars` used for all number formatting — no heap, no `printf`.

## Entities Documented

- [[Pin]] — GPIO wrapper
- [[RingBuffer]] — ring buffer
- [[ByteStream and Print Mixin]] — stream concept + print helpers
- [[SerialPort]] — serial facade
- [[Uart INT]] — interrupt UART
- [[Uart DMA]] — DMA UART
- [[Timebase]] — millis/micros
- [[LineReader]] — line input
- [[Startup SAMC21]] — reset + vector table
- [[Syscalls]] — newlib bridge

## See Also

- [[Application Bring-Up Source]] — how the library is wired together in main.cpp
- [[SERCOM UART Configuration]] — hardware background
- [[Clock System]] — clock setup that library code depends on
