---
title: SERCOM UART Configuration
type: concept
tags: [sercom, uart, usart, firmware, samc21]
sources: [firmware-core-library, application-bring-up, samc21-datasheet-ch31-sercom-usart]
created: 2026-05-05
updated: 2026-05-05
---

# SERCOM UART Configuration

How a SERCOM instance is configured as USART in the firmware. Based on `uart_int.hpp` and
`uart_dma.hpp`.

## SercomTraits<N>

Static trait struct parameterized on SERCOM index (0–5):

```cpp
template <> struct SercomTraits<5> {
    static inline Sercom* sercom() { return SERCOM5; }
    static constexpr IRQn_Type irqn         = SERCOM5_IRQn;
    static constexpr uint32_t  apb_mask     = MCLK_APBCMASK_SERCOM5;
    static constexpr uint8_t   gclk_id_core = SERCOM5_GCLK_ID_CORE;
    static constexpr uint8_t   dmac_id_rx   = SERCOM5_DMAC_ID_RX;
    static constexpr uint8_t   dmac_id_tx   = SERCOM5_DMAC_ID_TX;
};
```

## UartPinout<TxPin, RxPin, Peripheral, txpo, rxpo>

Encodes pin assignment and pad mapping:

```cpp
// SERCOM5 on PB30(TX)/PB31(RX), peripheral D, TXPO=0, RXPO=1
using Serial5Pinout = UartPinout<
    sam::gpio::Pin<sam::gpio::Bank::B, 30>,   // TX
    sam::gpio::Pin<sam::gpio::Bank::B, 31>,   // RX
    sam::gpio::Peripheral::D,                  // PMUX value D = 3
    0,   // TXPO: TX on PAD[0]
    1>;  // RXPO: RX on PAD[1]
```

`apply()` sets TX pin high (mark/idle state) before switching to peripheral mux, preventing
spurious start bits on the wire during init.

## TXPO / RXPO Values

`TXPO` and `RXPO` fields in CTRLA select which SERCOM pad is used for TX/RX.

### TXPO

| TXPO | TX | Extra | Notes |
|------|----|-------|-------|
| 0x0 | PAD[0] | XCK=PAD[1] | Standard no-flow-control TX |
| 0x1 | PAD[2] | XCK=PAD[3] | TX on PAD[2] |
| 0x2 | PAD[0] | RTS=PAD[2], CTS=PAD[3] | Hardware flow control |
| 0x3 | PAD[0] | XCK=PAD[1], RTS=PAD[2] | RS-485 mode |

### RXPO

| RXPO | RX pad |
|------|--------|
| 0x0 | PAD[0] |
| 0x1 | PAD[1] |
| 0x2 | PAD[2] |
| 0x3 | PAD[3] |

**Board config:** TXPO=0 (TX=PAD[0]=PB30), RXPO=1 (RX=PAD[1]=PB31). No flow control needed.

## Init Sequence

```cpp
// 1. Enable APB clock
MCLK->APBCMASK.reg |= Traits::apb_mask;

// 2. Connect GCLK to SERCOM core
GCLK->PCHCTRL[Traits::gclk_id_core].reg = GCLK_PCHCTRL_GEN(gclk) | GCLK_PCHCTRL_CHEN;
while (!(GCLK->PCHCTRL[Traits::gclk_id_core].reg & GCLK_PCHCTRL_CHEN));

// 3. Software reset
u.CTRLA.reg = SERCOM_USART_CTRLA_SWRST;
while (u.SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_SWRST);
while (u.CTRLA.reg & SERCOM_USART_CTRLA_SWRST);

// 4. Baud rate (16x oversampling, fractional)
uint64_t br = 65536ULL * (F_CPU - 16 * baud) / F_CPU;
u.BAUD.reg = (uint16_t)br;

// 5. CTRLA
u.CTRLA.reg = SERCOM_USART_CTRLA_DORD          // LSB first
            | SERCOM_USART_CTRLA_MODE(1)         // internal clock
            | SERCOM_USART_CTRLA_RXPO(rxpo)
            | SERCOM_USART_CTRLA_TXPO(txpo);

// 6. CTRLB
u.CTRLB.reg = SERCOM_USART_CTRLB_RXEN
            | SERCOM_USART_CTRLB_TXEN
            | SERCOM_USART_CTRLB_CHSIZE(0);  // 8 bits
while (u.SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_CTRLB);

// 7. Enable
u.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
while (u.SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_ENABLE);
```

## Baud Rate Formula

16x oversampling (arithmetic mode):

```
BAUD_reg = 65536 × (1 − 16 × f_baud / f_ref)
```

where `f_ref` is the GCLK frequency feeding the SERCOM core.

| f_ref | baud | BAUD_reg |
|-------|------|----------|
| 48 MHz | 1,000,000 | 43691 (0xAAAA) |
| 48 MHz | 115,200 | 63019 (0xF5EB) |
| 48 MHz | 9,600 | 65427 (0xFF93) |

## Register Reference (Datasheet Ch.31)

### CTRLA Key Bits

| Bits | Name | Typical UART value |
|------|------|-------------------|
| 30 | DORD | 1 = LSB first |
| 28 | CMODE | 0 = asynchronous |
| 27:24 | FORM | 0 = no parity; 1 = with parity |
| 21:20 | RXPO | pad select (see table above) |
| 17:16 | TXPO | pad select (see table above) |
| 15:13 | SAMPR | 0 = 16× arithmetic |
| 4:2 | MODE | 0x1 = internal clock |
| 1 | ENABLE | 1 to enable |
| 0 | SWRST | 1 to reset |

### CTRLB Key Bits (Enable-Protected)

| Bits | Name | Description |
|------|------|-------------|
| 17 | RXEN | Receive enable |
| 16 | TXEN | Transmit enable |
| 6 | PMODE | Parity: 0=even, 1=odd |
| 5 | SBMODE | Stop bits: 0=one, 1=two |
| 2:0 | CHSIZE | 0=8-bit; 1=9-bit; 5/6/7=5/6/7-bit |

### STATUS Bits

| Bit | Name | Description |
|-----|------|-------------|
| 2 | BUFOVF | RX buffer overflow |
| 1 | FERR | Frame error |
| 0 | PERR | Parity error |

### SYNCBUSY Bits

| Bit | Name | Wait after |
|-----|------|-----------|
| 2 | CTRLB | CTRLB write |
| 1 | ENABLE | ENABLE set/clear |
| 0 | SWRST | SWRST write |

Three operations require a SYNCBUSY wait: SWRST, CTRLB write, and ENABLE write.
Missing these waits causes undefined behavior — the register write is silently ignored.

## Pin Multiplexing

SERCOM5 pads and peripheral mux on the J18A package:

| Signal | Pin | Peripheral Mux |
|--------|-----|----------------|
| SERCOM5/PAD[0] | PB30 | D |
| SERCOM5/PAD[1] | PB31 | D |

## See Also

- [[Uart INT]] — interrupt-driven implementation
- [[Uart DMA]] — DMA-driven implementation
- [[Clock System]] — GCLK setup that feeds the SERCOM
- [[Pin]] — pin mux setup (`set_peripheral`)
- [[I/O Multiplexing]] — SERCOM5 PAD-to-pin table (PB30/PB31)
- [[DMA Controller]] — DRE/RXC DMA triggers
- [[SAMC21 Datasheet Ch. 31 — SERCOM USART]] — register-level reference
