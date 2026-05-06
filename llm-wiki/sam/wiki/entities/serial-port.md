---
title: SerialPort
type: entity
tags: [uart, firmware, template, facade]
sources: [firmware-core-library]
created: 2026-05-05
updated: 2026-05-05
---

# SerialPort

Thin facade combining a UART driver with [[ByteStream and Print Mixin]].
Defined in `src/core/serial_port.hpp`.

## Template Signature

```cpp
template <typename UartT>
class SerialPort : public ByteStreamPrintMixin<SerialPort<UartT>>;
```

## API

```cpp
void   init(uint32_t baud, uint8_t gclk = 0);
bool   ready() const;          // true if init() completed successfully

// ByteStream interface (also satisfies the ByteStream concept):
size_t write(const uint8_t *data, size_t size);
size_t read(uint8_t *data, size_t size);
size_t available();

// Additional:
void   flush();                // blocks until tx_idle()
void   irq_handler();         // forward to uart_.irq_handler()

// Inherited from ByteStreamPrintMixin:
size_t print(const char*);
size_t print(uint32_t, PrintBase = Dec);
size_t print(int32_t, PrintBase = Dec);
// ... all integer overloads ...
size_t newline();              // writes "\r\n"
```

## The Global Serial Instance

Declared in `src/serial.hpp`, defined in `src/interrupts.cpp`:

```cpp
using SerialType = SerialPort<
    UartINT<SercomTraits<5>,
            UartPinout<UartTxPin, UartRxPin, sam::gpio::Peripheral::D, 0, 1>,
            5, 8>>;

extern SerialType Serial;
```

- SERCOM5, PB30(TX)/PB31(RX), peripheral D, TXPO=0, RXPO=1
- RX buffer: 2^5 = 32 slots; TX buffer: 2^8 = 256 slots

## Typical Usage

```cpp
Serial.init(1000000);          // 1 Mbaud, GCLK0
Serial.print("Value: ");
Serial.print(my_value, PrintBase::Hex);
Serial.newline();
Serial.flush();                // ensure all bytes are sent
```

## newlib Integration

`syscalls.cpp` routes `_write`/`_read` to `Serial`, so `printf` and `std::cout` work out of the box — but they pull in heavy formatting code. Prefer `Serial.print()` for embedded use.

## See Also

- [[Uart INT]] — the default underlying driver
- [[Uart DMA]] — alternative driver for high-throughput scenarios
- [[ByteStream and Print Mixin]] — print API detail
- [[Syscalls]] — newlib bridge
