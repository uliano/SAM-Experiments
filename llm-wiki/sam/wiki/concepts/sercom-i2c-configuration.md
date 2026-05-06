---
title: SERCOM I2C Configuration
type: concept
tags: [sercom, i2c, twi, firmware, samc21]
sources: [samc21-datasheet-ch33-sercom-i2c]
created: 2026-05-05
updated: 2026-05-05
---

# SERCOM I2C Configuration

I2C master and slave configuration on the SAMC21 SERCOM. PAD[0]=SDA,
PAD[1]=SCL (fixed). Enable-protected CTRLA/CTRLB/BAUD require configuration
before ENABLE=1.

## Master Init — Standard Mode, 100 kHz (SERCOM2)

```cpp
// SERCOM2 I2C master, 100 kHz, SDAHOLD=75ns, SDA=PA08/PAD[0], SCL=PA09/PAD[1]

void i2c_master_init(void) {
    // 1. APB clock
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_SERCOM2;

    // 2. GCLK for SERCOM2 CORE (GCLK0 = 48 MHz)
    GCLK->PCHCTRL[SERCOM2_GCLK_ID_CORE].reg =
        GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[SERCOM2_GCLK_ID_CORE].reg & GCLK_PCHCTRL_CHEN));

    // 3. Software reset
    SERCOM2->I2CM.CTRLA.reg = SERCOM_I2CM_CTRLA_SWRST;
    while (SERCOM2->I2CM.SYNCBUSY.reg & SERCOM_I2CM_SYNCBUSY_SWRST);

    // 4. Configure (enable-protected, ENABLE=0 after reset)
    SERCOM2->I2CM.CTRLA.reg =
        SERCOM_I2CM_CTRLA_MODE_I2C_MASTER      // MODE=0x5
        | SERCOM_I2CM_CTRLA_SPEED(0x0)         // Standard/Fast up to 400 kHz
        | SERCOM_I2CM_CTRLA_SDAHOLD(0x1)       // SDA hold 50-100 ns
        | SERCOM_I2CM_CTRLA_INACTOUT(0x1);     // SMBus inactive timeout (optional)

    // BAUD = (48MHz / 100kHz - 10 - 48MHz*100ns) / 2 ≈ 232
    // T_RISE ≈ 100 ns for typical 100 kHz board
    SERCOM2->I2CM.BAUD.reg = SERCOM_I2CM_BAUD_BAUD(232);

    // 5. Enable
    SERCOM2->I2CM.CTRLA.reg |= SERCOM_I2CM_CTRLA_ENABLE;
    while (SERCOM2->I2CM.SYNCBUSY.reg & SERCOM_I2CM_SYNCBUSY_ENABLE);

    // 6. Force bus state to IDLE (reset leaves it UNKNOWN)
    SERCOM2->I2CM.STATUS.reg = SERCOM_I2CM_STATUS_BUSSTATE(0x1);
    while (SERCOM2->I2CM.SYNCBUSY.reg & SERCOM_I2CM_SYNCBUSY_SYSOP);
}
```

## BAUD Formula

```
f_SCL = f_GCLK / (10 + 2×BAUD + f_GCLK×T_RISE)   [BAUDLOW=0]
BAUD  = (f_GCLK/f_SCL - 10 - f_GCLK×T_RISE) / 2
```

| Mode | f_SCL | f_GCLK=48MHz | T_RISE | BAUD |
|------|-------|-------------|--------|------|
| Standard | 100 kHz | 48 MHz | 100 ns | 232 |
| Fast | 400 kHz | 48 MHz | 80 ns | 53 |
| Fast-mode Plus | 1 MHz | 48 MHz | 60 ns | 19 |

## Master Write — Polled

```cpp
bool i2c_master_write(uint8_t addr7, const uint8_t *buf, size_t len) {
    // Start write: ADDR bit 0 = 0 (write direction)
    SERCOM2->I2CM.ADDR.reg = SERCOM_I2CM_ADDR_ADDR(addr7 << 1);
    while (SERCOM2->I2CM.SYNCBUSY.reg & SERCOM_I2CM_SYNCBUSY_SYSOP);

    // Check address ACK
    while (!(SERCOM2->I2CM.INTFLAG.reg & SERCOM_I2CM_INTFLAG_MB));
    if (SERCOM2->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_RXNACK) {
        SERCOM2->I2CM.CTRLB.reg |= SERCOM_I2CM_CTRLB_CMD(0x3);  // STOP
        return false;
    }

    // Send data bytes
    for (size_t i = 0; i < len; i++) {
        SERCOM2->I2CM.DATA.reg = buf[i];
        while (SERCOM2->I2CM.SYNCBUSY.reg & SERCOM_I2CM_SYNCBUSY_SYSOP);
        while (!(SERCOM2->I2CM.INTFLAG.reg & SERCOM_I2CM_INTFLAG_MB));
        if (SERCOM2->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_RXNACK) {
            SERCOM2->I2CM.CTRLB.reg |= SERCOM_I2CM_CTRLB_CMD(0x3);
            return false;
        }
    }

    // Issue STOP
    SERCOM2->I2CM.CTRLB.reg |= SERCOM_I2CM_CTRLB_CMD(0x3);
    return true;
}
```

## Master Read — Polled

```cpp
bool i2c_master_read(uint8_t addr7, uint8_t *buf, size_t len) {
    // Start read: ADDR bit 0 = 1 (read direction)
    SERCOM2->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT;  // ACK next
    SERCOM2->I2CM.ADDR.reg = SERCOM_I2CM_ADDR_ADDR((addr7 << 1) | 1u);
    while (SERCOM2->I2CM.SYNCBUSY.reg & SERCOM_I2CM_SYNCBUSY_SYSOP);

    // Check address ACK
    while (!(SERCOM2->I2CM.INTFLAG.reg & SERCOM_I2CM_INTFLAG_SB));
    if (SERCOM2->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_RXNACK) {
        SERCOM2->I2CM.CTRLB.reg |= SERCOM_I2CM_CTRLB_CMD(0x3);
        return false;
    }

    // Receive bytes
    for (size_t i = 0; i < len; i++) {
        if (i == len - 1) {
            // NACK + STOP on last byte
            SERCOM2->I2CM.CTRLB.reg |= SERCOM_I2CM_CTRLB_ACKACT;
        }
        SERCOM2->I2CM.CTRLB.reg |= SERCOM_I2CM_CTRLB_CMD(0x2);  // read byte
        while (SERCOM2->I2CM.SYNCBUSY.reg & SERCOM_I2CM_SYNCBUSY_SYSOP);
        while (!(SERCOM2->I2CM.INTFLAG.reg & SERCOM_I2CM_INTFLAG_SB));
        buf[i] = (uint8_t)SERCOM2->I2CM.DATA.reg;
    }

    SERCOM2->I2CM.CTRLB.reg |= SERCOM_I2CM_CTRLB_CMD(0x3);  // STOP
    return true;
}
```

## Slave Init

```cpp
// SERCOM0 I2C slave, address 0x42, 7-bit, AACKEN=1 (auto-ACK on address match)

void i2c_slave_init(void) {
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_SERCOM0;
    GCLK->PCHCTRL[SERCOM0_GCLK_ID_CORE].reg =
        GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[SERCOM0_GCLK_ID_CORE].reg & GCLK_PCHCTRL_CHEN));

    SERCOM0->I2CS.CTRLA.reg = SERCOM_I2CS_CTRLA_SWRST;
    while (SERCOM0->I2CS.SYNCBUSY.reg & SERCOM_I2CS_SYNCBUSY_SWRST);

    SERCOM0->I2CS.CTRLA.reg =
        SERCOM_I2CS_CTRLA_MODE_I2C_SLAVE       // MODE=0x4
        | SERCOM_I2CS_CTRLA_SDAHOLD(0x1)       // SDA hold 50-100 ns
        | SERCOM_I2CS_CTRLA_SCLSM;             // stretch after ACK bit

    SERCOM0->I2CS.CTRLB.reg =
        SERCOM_I2CS_CTRLB_AACKEN;              // auto-ACK address match

    // Address (7-bit stored in ADDR[7:1])
    SERCOM0->I2CS.ADDR.reg = SERCOM_I2CS_ADDR_ADDR(0x42 << 1);

    // Enable interrupts
    SERCOM0->I2CS.INTENSET.reg =
        SERCOM_I2CS_INTENSET_AMATCH
        | SERCOM_I2CS_INTENSET_DRDY
        | SERCOM_I2CS_INTENSET_PREC
        | SERCOM_I2CS_INTENSET_ERROR;

    SERCOM0->I2CS.CTRLA.reg |= SERCOM_I2CS_CTRLA_ENABLE;
    while (SERCOM0->I2CS.SYNCBUSY.reg & SERCOM_I2CS_SYNCBUSY_ENABLE);

    NVIC_EnableIRQ(SERCOM0_IRQn);
}
```

## Slave ISR Pattern

```cpp
void SERCOM0_Handler(void) {
    uint8_t flags = SERCOM0->I2CS.INTFLAG.reg;

    if (flags & SERCOM_I2CS_INTFLAG_ERROR) {
        SERCOM0->I2CS.INTFLAG.reg = SERCOM_I2CS_INTFLAG_ERROR;
        // handle error
    }
    else if (flags & SERCOM_I2CS_INTFLAG_AMATCH) {
        // Address match: STATUS.DIR tells direction
        if (SERCOM0->I2CS.STATUS.reg & SERCOM_I2CS_STATUS_DIR) {
            // Master read — prepare first TX byte
            SERCOM0->I2CS.DATA.reg = tx_buf[0];
            tx_idx = 1;
        } else {
            // Master write — arm to receive
            rx_idx = 0;
        }
        SERCOM0->I2CS.CTRLB.reg = SERCOM_I2CS_CTRLB_CMD(0x3);  // ACK + prepare
    }
    else if (flags & SERCOM_I2CS_INTFLAG_DRDY) {
        if (SERCOM0->I2CS.STATUS.reg & SERCOM_I2CS_STATUS_DIR) {
            // Master read — send next byte or NACK
            if (SERCOM0->I2CS.STATUS.reg & SERCOM_I2CS_STATUS_RXNACK) {
                SERCOM0->I2CS.CTRLB.reg = SERCOM_I2CS_CTRLB_CMD(0x2);
            } else {
                SERCOM0->I2CS.DATA.reg = tx_buf[tx_idx++];
            }
        } else {
            // Master write — read received byte
            rx_buf[rx_idx++] = (uint8_t)SERCOM0->I2CS.DATA.reg;
            SERCOM0->I2CS.CTRLB.reg = SERCOM_I2CS_CTRLB_CMD(0x3);  // ACK
        }
    }
    else if (flags & SERCOM_I2CS_INTFLAG_PREC) {
        // STOP received — transaction complete
        SERCOM0->I2CS.INTFLAG.reg = SERCOM_I2CS_INTFLAG_PREC;
        on_transfer_complete(rx_buf, rx_idx);
    }
}
```

## DMA Master Write

```cpp
// Write N bytes to slave address using DMA (no polling)
void i2c_dma_write(uint8_t addr7, const uint8_t *buf, uint8_t n) {
    // Configure DMA descriptor: TRIGSRC=SERCOM2_DMAC_ID_TX, TRIGACT=BEAT
    // SRCADDR = buf + n (end address), DSTADDR = &SERCOM2->I2CM.DATA.reg, BTCNT=n

    // Start transaction with automatic length — hardware sends STOP after n bytes
    SERCOM2->I2CM.ADDR.reg =
        SERCOM_I2CM_ADDR_ADDR(addr7 << 1)
        | SERCOM_I2CM_ADDR_LENEN
        | SERCOM_I2CM_ADDR_LEN(n);
    while (SERCOM2->I2CM.SYNCBUSY.reg & SERCOM_I2CM_SYNCBUSY_SYSOP);
    // Enable DMA channel — it fires on MB interrupt (DRE)
}
```

## Bus State Recovery (BUSSTATE=UNKNOWN)

```cpp
// After reset or error, force bus to IDLE before first transaction
if ((SERCOM2->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_BUSSTATE_Msk) ==
     SERCOM_I2CM_STATUS_BUSSTATE(0x0)) {
    SERCOM2->I2CM.STATUS.reg = SERCOM_I2CM_STATUS_BUSSTATE(0x1);
    while (SERCOM2->I2CM.SYNCBUSY.reg & SERCOM_I2CM_SYNCBUSY_SYSOP);
}
```

## Key Facts

- PAD[0]=SDA, PAD[1]=SCL — fixed unlike SPI.
- ADDR bit 0 is the R/W flag (0=write, 1=read); the 7-bit address occupies bits 7:1.
- BUSSTATE=UNKNOWN after reset; write 0x1 to force IDLE before first transaction.
- Writing ADDR triggers the bus operation; poll SYNCBUSY.SYSOP afterward.
- ACKACT must be set to 1 before the last byte in a master read to send NACK+STOP.
- Smart mode (SMEN=1): ACK action triggers automatically on DATA read — reduces ISR code.
- Master SYNCBUSY.SYSOP: three bits trigger it — CMD, BUSSTATE, ADDR, DATA writes.
- Slave DATA is write-synchronized and read-synchronized (SYNCBUSY has only ENABLE/SWRST).
- GCLK_SERCOM_SLOW is shared across all SERCOM instances — configure once for SMBus use.
- LENERR (STATUS.10): slave NACKed before all LEN bytes were transferred in DMA mode.
- Writing ADDR clears STATUS.BUSERR and ARBLOST automatically.

## See Also

- [[SAMC21 Datasheet Ch.33 SERCOM I2C]] — full register reference (slave and master)
- [[I/O Multiplexing]] — SERCOM PAD to physical pin mapping
- [[DMA Controller]] — I2C DMA trigger (SERCOMx_DMAC_ID_TX)
- [[Clock System]] — GCLK_SERCOMx_CORE and GCLK_SERCOM_SLOW setup
