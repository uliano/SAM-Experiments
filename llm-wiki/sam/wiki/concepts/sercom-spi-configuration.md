---
title: SERCOM SPI Configuration
type: concept
tags: [sercom, spi, firmware, samc21]
sources: [samc21-datasheet-ch32-sercom-spi]
created: 2026-05-05
updated: 2026-05-05
---

# SERCOM SPI Configuration

SPI master/slave configuration on the SAMC21 SERCOM. Full-duplex 4-wire
(MOSI/MISO/SCK/SS̄). Enable-protected CTRLA/CTRLB/BAUD require configuration
before ENABLE=1.

## Master Init — Polled (Mode 0, 4 MHz, MSB first)

```cpp
// Example: SERCOM1 SPI master, DOPO=0x0 (DO=PAD0, SCK=PAD1, SS=PAD2)
//          DIPO=0x3 (DI=PAD3 = MISO), SPI Mode 0, 4 MHz, 8-bit

void spi_master_init(void) {
    // 1. APB clock
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_SERCOM1;

    // 2. GCLK for SERCOM1 CORE (GCLK0 = 48 MHz)
    GCLK->PCHCTRL[SERCOM1_GCLK_ID_CORE].reg =
        GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[SERCOM1_GCLK_ID_CORE].reg & GCLK_PCHCTRL_CHEN));

    // 3. Software reset
    SERCOM1->SPI.CTRLA.reg = SERCOM_SPI_CTRLA_SWRST;
    while (SERCOM1->SPI.SYNCBUSY.reg & SERCOM_SPI_SYNCBUSY_SWRST);

    // 4. Configure (enable-protected, ENABLE=0 after reset)
    SERCOM1->SPI.CTRLA.reg =
        SERCOM_SPI_CTRLA_MODE_SPI_MASTER    // MODE=0x3
        | SERCOM_SPI_CTRLA_DOPO(0x0)        // DO=PAD0, SCK=PAD1
        | SERCOM_SPI_CTRLA_DIPO(0x3)        // DI=PAD3 (MISO)
        | SERCOM_SPI_CTRLA_CPOL             // CPOL=0 (SPI Mode 0)
        // | SERCOM_SPI_CTRLA_CPHA          // CPHA=0
        // DORD: 0=MSB first (default)
        ;

    // BAUD = 48MHz / (2*4MHz) - 1 = 5
    SERCOM1->SPI.BAUD.reg = SERCOM_SPI_BAUD_BAUD(5);

    // CTRLB: 8-bit, no hardware SS, enable receiver
    SERCOM1->SPI.CTRLB.reg =
        SERCOM_SPI_CTRLB_CHSIZE(0)   // 8-bit
        | SERCOM_SPI_CTRLB_RXEN;
    while (SERCOM1->SPI.SYNCBUSY.reg & SERCOM_SPI_SYNCBUSY_CTRLB);

    // 5. Enable
    SERCOM1->SPI.CTRLA.reg |= SERCOM_SPI_CTRLA_ENABLE;
    while (SERCOM1->SPI.SYNCBUSY.reg & SERCOM_SPI_SYNCBUSY_ENABLE);
}
```

## Polled Transfer — Single Byte

```cpp
uint8_t spi_transfer(uint8_t tx) {
    while (!(SERCOM1->SPI.INTFLAG.reg & SERCOM_SPI_INTFLAG_DRE));
    SERCOM1->SPI.DATA.reg = tx;
    while (!(SERCOM1->SPI.INTFLAG.reg & SERCOM_SPI_INTFLAG_RXC));
    return (uint8_t)SERCOM1->SPI.DATA.reg;
}
```

## Polled Multi-byte Transfer with Software SS

```cpp
void spi_write_buf(const uint8_t *buf, size_t len) {
    pin_ss.low();  // Assert SS
    for (size_t i = 0; i < len; i++) {
        while (!(SERCOM1->SPI.INTFLAG.reg & SERCOM_SPI_INTFLAG_DRE));
        SERCOM1->SPI.DATA.reg = buf[i];
    }
    // Wait for last byte to finish shifting out
    while (!(SERCOM1->SPI.INTFLAG.reg & SERCOM_SPI_INTFLAG_TXC));
    pin_ss.high();  // Deassert SS
}
```

## BAUD Formula

```
f_SCK = GCLK_SERCOMx_CORE / (2 × (BAUD + 1))
BAUD  = GCLK / (2 × f_SCK) - 1
```

| Target f_SCK | GCLK=48MHz | BAUD |
|-------------|-----------|------|
| 1 MHz | 48 | 23 |
| 4 MHz | 48 | 5 |
| 8 MHz | 48 | 2 |
| 12 MHz | 48 | 1 |
| 24 MHz | 48 | 0 |

## SPI Modes (CPOL / CPHA)

| SPI Mode | CPOL | CPHA | SCK idle | Data sampled |
|----------|------|------|---------|-------------|
| 0 | 0 | 0 | Low | Rising edge |
| 1 | 0 | 1 | Low | Falling edge |
| 2 | 1 | 0 | High | Falling edge |
| 3 | 1 | 1 | High | Rising edge |

## DOPO Pin Mapping (Master: DO=MOSI, DI=MISO)

| DOPO | MOSI | SCK | SS̄ (MSSEN hw) |
|------|------|-----|--------------|
| 0x0 | PAD[0] | PAD[1] | PAD[2] |
| 0x1 | PAD[2] | PAD[3] | PAD[1] |
| 0x2 | PAD[3] | PAD[1] | PAD[2] |
| 0x3 | PAD[0] | PAD[3] | PAD[1] |

DIPO selects the MISO pad independently (PAD[0]–PAD[3]).

## Hardware SS̄ (MSSEN=1)

```cpp
// Add to CTRLB for automatic hardware SS control
SERCOM1->SPI.CTRLB.reg |= SERCOM_SPI_CTRLB_MSSEN;
// SS is asserted 1 baud cycle before each byte and
// deasserted 1 baud cycle after each byte.
// Back-to-back bytes: SS rises briefly between frames.
```

## DMA Transfer (TX only, no RX)

```cpp
// Set up DMA descriptor for SPI TX
// TRIGSRC = SERCOMx_DMAC_ID_TX, TRIGACT = BEAT
// SRCADDR = end of buffer, DSTADDR = &SERCOM1->SPI.DATA.reg
// Disable RX if not needed to avoid BUFOVF
```

## Key Facts

- CTRLA (except ENABLE/SWRST), CTRLB (except RXEN), BAUD, ADDR are enable-protected.
- Three SYNCBUSY points: poll after SWRST, ENABLE, and CTRLB.RXEN writes.
- Writing CTRLB while SYNCBUSY.CTRLB=1 causes an APB bus error.
- DRE is read-only (cleared by writing DATA); RXC is read-only (cleared by reading DATA).
- TXC: set when shift register is empty and no new data in DATA (all bytes shifted out).
- MSSEN=0 master: manually drive SS̄ via GPIO; deassert only after TXC is set.
- IBON=0 (default): BUFOVF propagates through receive FIFO — read stale data before ERROR fires.
- IBON=1: BUFOVF fires immediately; read RxDATA until RXC goes low to drain.
- Slave first character issue: first byte after SS̄ asserted is from shift register.
  Set PLOADEN=1 to preload before transaction begins.

## See Also

- [[SAMC21 Datasheet Ch.32 SERCOM SPI]] — full register reference, pin tables
- [[I/O Multiplexing]] — SERCOM PAD to physical pin mapping
- [[DMA Controller]] — SERCOM SPI DMAC_ID_TX/RX trigger values
- [[Clock System]] — GCLK_SERCOMx_CORE setup, APBC mask
