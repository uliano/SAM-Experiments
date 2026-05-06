---
title: RingBuffer
type: entity
tags: [data-structure, firmware, template]
sources: [firmware-core-library]
created: 2026-05-05
updated: 2026-05-05
---

# RingBuffer

Power-of-2 fixed-size ring buffer with overwrite semantics. Defined in `src/core/ring_buffer.hpp`.

## Template Signature

```cpp
template <typename T, uint8_t N>
class RingBuffer;
```

- `T`: element type
- `N`: **exponent** — storage size = `2^N`, usable capacity = `2^N - 1`

Examples: `RingBuffer<uint8_t, 8>` → 256 bytes storage, 255 usable.

## Size

```cpp
static constexpr size_t storage_size   = 1u << N;   // total allocated slots
static constexpr size_t capacity_value = storage_size - 1;  // max elements
static constexpr size_t capacity();   // returns capacity_value
```

One slot is always sacrificed so that `head == tail` unambiguously means empty.

## Key Methods

```cpp
bool push(const T& value);     // returns true if oldest element was overwritten
bool push(T&& value);
template<typename... Args>
bool emplace(Args&&... args);  // in-place construct

bool pop(T& out);              // returns false if empty; moves element out
bool peek(T& out) const;       // non-destructive read
T*   front();                  // pointer to oldest element, nullptr if empty

size_t size() const;
bool   empty() const;
bool   full() const;
void   clear();
```

## Overwrite Semantics

When `push()` is called on a full buffer, the **oldest** entry is silently overwritten and the method returns `true`. The caller decides whether this matters:

```cpp
if (rx_.push(value))
    ++stats_.rx_overwrite_count;   // from uart_int.hpp
```

This is intentional for UART RX: prefer receiving new data over blocking the ISR.

## Thread Safety

**Not thread-safe.** Both [[Uart INT]] and [[Uart DMA]] protect all buffer accesses with [[IRQ Critical Section]] (PRIMASK save/restore). Do the same in any ISR-shared use.

## Masking

Index arithmetic uses `(index + 1) & (storage_size - 1)`. Because `storage_size` is a power of 2, this compiles to a single `AND` instruction — no branch, no division.

## See Also

- [[Uart INT]] — uses `RingBuffer<uint8_t, RxN>` and `RingBuffer<uint8_t, TxN>`
- [[Uart DMA]] — same for software RX staging
- [[IRQ Critical Section]] — how concurrent access is protected
