---
title: Application Bring-Up Source
type: source
tags: [firmware, samc21, bring-up, application]
sources: [application-bring-up]
created: 2026-05-05
updated: 2026-05-05
---

# Application Bring-Up Source

The application layer on top of the core library. Files in `src/` (not `src/core/`).

## Files

| File | Description |
|------|-------------|
| `main.cpp` | Entry point: sys_init → Timebase → Serial → QuartzTest → TcTest → WFI loop |
| `init.hpp` | `sys_init()` — flash wait states + OSC48M/GCLK0 setup |
| `pins.hpp` | Board pin aliases (`UartTxPin`, `UartRxPin`, `Led0Pin`, `Btn0Pin`, `SuspectPa04Pin`) |
| `serial.hpp` | `Serial` global: `SerialPort<UartINT<SercomTraits<5>, UartPinout<...>>>` |
| `quartz_test.hpp` | `QuartzTest` — crystal startup + diagnostic report |

## Boot Sequence

```
irq_handler_reset()          [startup_samc21.c]
  copy .data, zero .bss, __libc_init_array, set VTOR
  → main()
    sys_init()               NVM wait states, OSC48M@48MHz, GCLK0
    Timebase::init()         SysTick @ 1 ms
    Serial.init(1000000)     SERCOM5 PB30/PB31, GCLK0, IRQ enabled
    QuartzTest::init()       start XOSC 24MHz, wait ≤100ms; (XOSC32K if build flag)
    QuartzTest::report_once() print crystal status report
    loop: __WFI()            sleep until next interrupt
```

## Serial Instance

`Serial` is declared in `serial.hpp` / defined in `interrupts.cpp` (not in core):

```cpp
SerialPort<UartINT<SercomTraits<5>,
    UartPinout<UartTxPin, UartRxPin, sam::gpio::Peripheral::D, 0, 1>,
    5, 8>>
```

- SERCOM5, peripheral mux D, TXPO=0 (PAD[0]=TX), RXPO=1 (PAD[1]=RX)
- RX ring buffer: 2^5 = 32 bytes (exponent 5); TX ring buffer: 2^8 = 256 bytes (exponent 8)
- PB30 = TX (PAD[0]), PB31 = RX (PAD[1])

## GCLK Assignments After Boot

| Generator | Source | Frequency | Status |
|-----------|--------|-----------|--------|
| GCLK0 | OSC48M | 48 MHz | always active |
| GCLK1 | XOSC | 24 MHz | only if crystal OK |
| GCLK2 | XOSC/64 | 375 kHz | only if crystal OK |
| GCLK3 | XOSC32K | 32.768 kHz | only if build flag + crystal OK |

## Board Pin Assignments

| Alias | Pin | Function |
|-------|-----|----------|
| `UartTxPin` | PB30 | SERCOM5 PAD[0] TX, peripheral D |
| `UartRxPin` | PB31 | SERCOM5 PAD[1] RX, peripheral D |
| `Led0Pin` | PB23 | Active-high bench LED |
| `Btn0Pin` | PB22 | Active-high button, external pull-down |
| `SuspectPa04Pin` | PA04 | Under investigation — do not use in generic bring-up |

## Notes

- PA03 and PA04 are VREF conditioning pins (22 Ω series + 22 nF to ground). Treat as capacitively loaded analog/reference pins.

## See Also

- [[Firmware Core Library]] — the library this wraps
- [[Clock System]] — detail on sys_init and QuartzTest clock setup
- [[Uart INT]] — the UART implementation used by Serial
- [[QuartzTest]] — crystal test and report entity
