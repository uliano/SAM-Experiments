---
title: LineReader
type: entity
tags: [firmware, template, input, uart]
sources: [firmware-core-library]
created: 2026-05-05
updated: 2026-05-05
---

# LineReader

Non-blocking line input reader with echo, backspace, and timeout support.
Defined in `src/core/line_reader.hpp`.

## Template Signature

```cpp
template <typename StreamT>
class LineReader;
```

`StreamT` must provide: `available()`, `read(uint8_t*, size_t)`, `write(const uint8_t*, size_t)`,
`ready()`, `newline()`. [[SerialPort]] satisfies all of these.

## Construction

```cpp
char buf[64];
LineReader reader(Serial, buf, sizeof(buf), /*echo=*/true);
```

`capacity` must be ≥ 2 (1 byte for content + 1 for null terminator). If storage is invalid,
`poll()` always returns `InvalidArgs`.

## poll()

```cpp
PollResult poll();   // returns { Status, length }

enum class Status : uint8_t {
    Pending,       // line not yet complete
    Ready,         // line complete, call c_str()
    Overflow,      // line too long for buffer
    Timeout,       // timeout elapsed since first character
    InvalidArgs,   // buffer too small or null
};
```

Call `poll()` repeatedly from the main loop — it is non-blocking. When `Ready`, call `c_str()`
to get the null-terminated line. Then call `consume_line()` to reset.

## Character Handling

- Accepted: `0x20`–`0x7e` (printable ASCII) + `\t`
- Line terminators: `\r` or `\n` — `\r\n` sequences are collapsed (skip `\n` after `\r`)
- Backspace: `\b` (0x08) or DEL (0x7f) — removes last char, echoes `\b \b` sequence
- Characters outside accepted range are silently discarded

## Echo

When `echo=true` (default):
- Accepted chars are echoed back immediately
- Backspace is echoed as `\b space \b`
- Line terminator triggers `stream_.newline()` (writes `\r\n`)

## Timeout

```cpp
reader.set_timeout_ms(5000);   // 5 second timeout
```

Timeout starts on **first character received**, not on first `poll()` call. Returns
`Status::Timeout` with the number of characters received so far.

If `timeout_ms == 0` (default), timeout is disabled.

## Usage Example

```cpp
char cmd_buf[64];
LineReader reader(Serial, cmd_buf, sizeof(cmd_buf));
reader.set_timeout_ms(10000);

// In main loop:
auto result = reader.poll();
if (result.status == LineReader<SerialType>::Status::Ready) {
    process_command(reader.c_str());
    reader.consume_line();
}
```

## See Also

- [[SerialPort]] — the typical stream argument
- [[Timebase]] — provides `millis()` for timeout tracking
- [[ByteStream and Print Mixin]] — the write/newline interface used for echo
