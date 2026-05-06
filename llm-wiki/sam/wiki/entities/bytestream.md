---
title: ByteStream and Print Mixin
type: entity
tags: [firmware, template, concepts, printing, c++20]
sources: [firmware-core-library]
created: 2026-05-05
updated: 2026-05-05
---

# ByteStream and Print Mixin

C++20 concept defining a byte-oriented stream interface, plus a CRTP print mixin.
Defined in `src/core/bytestream.hpp`.

## ByteStream Concept

```cpp
template <typename StreamT>
concept ByteStream =
    requires(StreamT stream, const uint8_t *tx, uint8_t *rx, size_t size) {
        { stream.write(tx, size) } -> std::same_as<size_t>;
        { stream.read(rx, size)  } -> std::same_as<size_t>;
        { stream.available()     } -> std::same_as<size_t>;
    };
```

Any type satisfying this concept can use all `bytestream_*` free functions below.

## Free Functions

```cpp
bytestream_write_cstr(stream, "text");     // null-terminated string
bytestream_print(stream, "text");          // same
bytestream_print(stream, uint8_t v, PrintBase::Dec);
bytestream_print(stream, uint16_t v, PrintBase::Hex);
bytestream_print(stream, uint32_t v, PrintBase::Bin);
bytestream_print(stream, int32_t v);      // signed decimal only (Hex/Bin print unsigned bits)
bytestream_print(stream, uint64_t v);
bytestream_print(stream, int64_t v);
bytestream_newline(stream);               // writes "\r\n"
```

All integer formatting uses `std::to_chars` — **no heap allocation, no printf**.

## PrintBase Enum

```cpp
enum class PrintBase : uint8_t { Bin = 2, Dec = 10, Hex = 16 };
```

- `Hex` output: prefixed with `0x`, uppercase letters (e.g. `0xFF`)
- `Bin` output: prefixed with `0b`
- Signed integers in `Hex`/`Bin` mode: cast to unsigned first (bit-pattern output)

## ByteStreamPrintMixin (CRTP)

```cpp
template <typename DerivedT>
class ByteStreamPrintMixin {
public:
    size_t print(const char *text);
    size_t print(uint8_t v, PrintBase base = PrintBase::Dec);
    size_t print(int32_t v, PrintBase base = PrintBase::Dec);
    // ... all integer types ...
    size_t newline();
};
```

Inherit via CRTP to get `.print()` / `.newline()` on any stream type:

```cpp
class MyStream : public ByteStreamPrintMixin<MyStream> {
    size_t write(const uint8_t*, size_t);
    size_t read(uint8_t*, size_t);
    size_t available();
};
```

[[SerialPort]] uses this — `Serial.print(42)`, `Serial.print("hello")`, `Serial.newline()`.

## Usage Example

```cpp
Serial.print("Value: ");
Serial.print(sensor_reading, PrintBase::Hex);
Serial.newline();
```

## See Also

- [[SerialPort]] — the primary concrete stream
- [[LineReader]] — reads from any ByteStream-like object
