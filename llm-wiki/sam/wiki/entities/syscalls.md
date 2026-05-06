---
title: Syscalls
type: entity
tags: [firmware, newlib, syscalls]
sources: [firmware-core-library]
created: 2026-05-05
updated: 2026-05-05
---

# Syscalls

Minimal newlib syscall stubs bridging file I/O to the [[SerialPort]] global.
Defined in `src/core/syscalls.cpp`.

## Implemented Stubs

| Function | Behavior |
|----------|----------|
| `_write(fd, ptr, len)` | fd 1 (stdout) or 2 (stderr) → `Serial.write()`; other fds → `EBADF` |
| `_read(fd, ptr, len)` | fd 0 (stdin) → `Serial.read()`; other fds → `EBADF` |
| `_close(fd)` | fd 0–2 → success (0); others → `EBADF` |
| `_isatty(fd)` | fd 0–2 → 1 (yes, it's a terminal); others → `EBADF` |
| `_fstat(fd, st)` | sets `st_mode = S_IFCHR` for fd 0–2 |
| `_lseek(fd, ...)` | always `ESPIPE` (cannot seek a character device) |

## Effect

- `printf(...)` → `_write` → `Serial.write()` — works, but pulls in heavy libc formatting.
- `scanf(...)` → `_read` → `Serial.read()` — works, but blocks.
- For embedded use, prefer `Serial.print()` directly ([[ByteStream and Print Mixin]]).

## Serial Readiness Check

`_write` and `_read` check `Serial.ready()` before attempting I/O. If `Serial` is not yet
initialized, `_write` silently discards (returns `len` to not signal an error), and `_read`
returns 0 (no bytes available).

## See Also

- [[SerialPort]] — the Serial instance used
- [[Startup SAMC21]] — `__libc_init_array()` is called before `main()`, before `Serial.init()`
