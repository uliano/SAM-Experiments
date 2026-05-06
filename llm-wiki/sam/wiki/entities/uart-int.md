---
title: Uart INT
type: entity
tags: [uart, sercom, interrupt, firmware, template]
sources: [firmware-core-library]
created: 2026-05-05
updated: 2026-05-05
---

# Uart INT

Interrupt-driven UART over SERCOM USART. Defined in `src/core/uart_int.hpp` as a partial
specialization of `Uart<UartMode::Interrupt, Traits, Pinout, RxN, TxN>`.

## Template Parameters

```cpp
Uart<UartMode::Interrupt, Traits, Pinout, RxN, TxN>
// convenience alias:
UartINT<Traits, Pinout, RxN, TxN>
```

- `Traits`: `SercomTraits<N>` (N = 0..5) — provides SERCOM pointer, IRQn, APB mask, GCLK ID
- `Pinout`: `UartPinout<TxPin, RxPin, PeripheralMux, txpo, rxpo>`
- `RxN`: RX ring buffer size exponent (capacity = 2^RxN − 1)
- `TxN`: TX ring buffer size exponent (capacity = 2^TxN − 1)

## Initialization

```cpp
uart.init(baud, gclk_generator = 0);
```

Steps performed:
1. `Pinout::apply()` — sets TX pin high (idle), then muxes TX and RX pins to peripheral
2. Enables APBC clock (`MCLK->APBCMASK |= Traits::apb_mask`)
3. Connects GCLK generator to SERCOM core (`GCLK->PCHCTRL[GCLK_ID_CORE]`)
4. SW reset and wait (`CTRLA.SWRST`)
5. Computes `BAUD` register: `65536 * (F_CPU − 16 * baud) / F_CPU`
6. Configures CTRLA: LSB first, internal clock (MODE=1), RXPO, TXPO
7. Configures CTRLB: 8-bit, RX+TX enabled
8. Enables SERCOM, then enables RXC + ERROR interrupts
9. `NVIC_ClearPendingIRQ` + `NVIC_EnableIRQ`

## Write (Non-Blocking)

```cpp
bool  write(uint8_t value);           // false if TX buffer full (drops, increments tx_drop_count)
size_t write(const uint8_t*, size_t); // writes as many as fit
size_t write(const char* text);
void  write_blocking(uint8_t);        // spins until accepted
void  write_blocking(const char*);
```

TX flow: `write()` → pushes to `tx_` → enables DRE interrupt → ISR drains `tx_` → DATA register.

## Read

```cpp
bool   read(uint8_t& out);   // false if RX empty
size_t available();           // bytes in RX buffer
```

## ISR Handler

Must be called from the SERCOM IRQ handler:

```cpp
void irq_handler();
```

Handles three interrupt flags:
- `ERROR`: reads `STATUS`, increments parity/frame/overflow counters, clears flags
- `RXC` (receive complete): reads `DATA`, pushes to `rx_`, increments `rx_overwrite_count` if buffer was full
- `DRE` (data register empty): pops from `tx_`, writes to `DATA`; if `tx_` empty, disables DRE interrupt

## Stats

```cpp
struct Stats {
    uint32_t rx_overwrite_count;
    uint32_t tx_drop_count;
    uint32_t parity_error_count;
    uint32_t frame_error_count;
    uint32_t hw_overflow_count;
};
Stats get_stats() const;
void  clear_stats();
```

## Other

```cpp
bool tx_idle() const;        // tx_ empty AND DRE flag set (safe to power down or flush)
bool is_initialized() const;
```

## Critical Sections

All read/write/stats operations that touch shared state use [[IRQ Critical Section]] (PRIMASK save/restore). The ISR itself runs with interrupts disabled, so the lock only matters for the non-ISR side.

## Baud Rate Formula

16x oversampling, fractional arithmetic (avoids floating point):

```
BAUD_reg = 65536 × (F_CPU − 16 × baud) / F_CPU
```

At F_CPU=48 MHz, baud=1000000: `65536 × (48000000 − 16000000) / 48000000 = 43691` (0xAAAA).

## See Also

- [[Uart DMA]] — DMA-driven alternative
- [[SerialPort]] — the facade wrapping this
- [[RingBuffer]] — used for TX and RX buffers
- [[SERCOM UART Configuration]] — hardware register detail
- [[IRQ Critical Section]] — critical section pattern
