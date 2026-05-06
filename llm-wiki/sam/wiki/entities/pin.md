---
title: Pin
type: entity
tags: [gpio, port, template, firmware]
sources: [firmware-core-library]
created: 2026-05-05
updated: 2026-05-05
---

# Pin

Zero-cost compile-time GPIO wrapper. Defined in `src/core/pin.hpp`, namespace `sam::gpio`.

## Template Signature

```cpp
template <Bank bank_index, uint8_t pin_index>
struct Pin;
```

- `bank_index`: `Bank::A`, `Bank::B`, or `Bank::C`
- `pin_index`: 0–31

All methods are `static inline` — no object, no overhead, direct register writes.

## Enums

```cpp
enum class Bank : uint8_t { A=0, B=1, C=2 };

enum class Peripheral : uint8_t { A=0, B=1, C=2, D=3, E=4, F=5, G=6, H=7, I=8 };

enum class Pull : uint8_t { None=0, Up, Down };

enum class DriveStrength : uint8_t { Normal=0, Strong };
```

## Direction and Output

```cpp
Pin<Bank::B, 23>::as_output();               // output, low, normal drive, input enable on
Pin<Bank::B, 23>::as_output(true);           // output, high
Pin<Bank::B, 22>::as_input();                // input, no pull
Pin<Bank::B, 22>::as_input(Pull::Up);        // input, pull-up (sets OUT=1, enables PULLEN)
Pin<Bank::B, 22>::as_input(Pull::Down);      // input, pull-down (sets OUT=0, enables PULLEN)
```

For pull-up: `OUT` bit is set to 1 first, then `PULLEN` is enabled — the PORT requires this order.
For pull-down: `OUT` bit is cleared to 0 first, then `PULLEN` is enabled.

## Read / Write

```cpp
Pin<Bank::B, 23>::set();          // OUTSET
Pin<Bank::B, 23>::clear();        // OUTCLR
Pin<Bank::B, 23>::toggle();       // OUTTGL
Pin<Bank::B, 23>::write(v);       // set or clear
bool v = Pin<Bank::B, 22>::read(); // reads IN register
```

## Peripheral Mux

```cpp
Pin<Bank::B, 30>::set_peripheral(Peripheral::D);          // PMUX + PMUXEN
Pin<Bank::B, 30>::set_peripheral(Peripheral::D,
    /*input_enable=*/true, Pull::None, DriveStrength::Normal);
Pin<Bank::B, 30>::clear_peripheral();                      // clears PMUXEN
```

PMUX register is shared between even/odd pins: even pin uses `PMUXE`, odd uses `PMUXO`.
`PMUXEN` in `PINCFG[n]` must be set for the mux to take effect.

## Other

```cpp
Pin<Bank::A, 5>::set_drive_strength(DriveStrength::Strong); // PORT_PINCFG_DRVSTR
Pin<Bank::A, 5>::set_input_enable(true);                    // PORT_PINCFG_INEN
Pin<Bank::A, 5>::set_sampled_input(true);                   // PORT.CTRL continuous sampling
bool out = Pin<Bank::A, 5>::is_output();                    // checks DIR register
```

## Usage Example

```cpp
using Led = sam::gpio::Pin<sam::gpio::Bank::B, 23>;
Led::as_output(false);   // configure
Led::set();              // on
Led::clear();            // off
Led::toggle();           // blink
```

## See Also

- [[Application Bring-Up Source]] — board pin aliases in `pins.hpp`
- [[SERCOM UART Configuration]] — how `UartPinout::apply()` uses `set_peripheral()`
