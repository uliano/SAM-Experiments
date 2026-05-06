---
title: CAN Configuration
type: concept
tags: [can, can-fd, firmware, samc21]
sources: [samc21-datasheet-ch34-can]
created: 2026-05-05
updated: 2026-05-05
---

# CAN Configuration

CAN and CAN-FD controller on the SAMC21 (SAM C21 only). Message RAM resides
in system SRAM and must be allocated and configured before the CAN is enabled.
Configuration registers are accessible only when both CCCR.INIT=1 and CCCR.CCE=1.

## Message RAM Layout

Define the layout in SRAM. Each field is a word-aligned address:

```cpp
// Place Message RAM in SRAM — allocate before CAN init
#define CAN0_RAM_BASE  0x20000000U  // or somewhere in SRAM

// Layout (for Classic CAN, 8-byte elements only):
//   Standard filters:  8 elements × 1 word  =  8 words  (0x00)
//   Extended filters:  4 elements × 2 words =  8 words  (0x08)
//   Rx FIFO 0:        16 elements × 4 words = 64 words  (0x10)
//   Tx Buffers:        4 elements × 4 words = 16 words  (0x50)
// Total: 96 words = 384 bytes
```

## CAN0 Init — Classic CAN, 500 kbps at 48 MHz GCLK

```cpp
void can0_init(void) {
    // 1. Clocks
    MCLK->AHBMASK.reg |= MCLK_AHBMASK_CAN0;
    GCLK->PCHCTRL[CAN0_GCLK_ID].reg =
        GCLK_PCHCTRL_GEN_GCLK5     // Use GCLK5 (must be configured ≤ CAN max)
        | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[CAN0_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));

    // 2. INIT should be 1 at reset; verify and set CCE
    while (!(CAN0->CCCR.reg & CAN_CCCR_INIT));
    CAN0->CCCR.reg |= CAN_CCCR_CCE;

    // 3. Nominal Bit Timing: 500 kbps @ 48 MHz GCLK
    //    t_q = (5+1)/48MHz = 125ns → 16 t_q/bit = 2000ns = 500kbps
    //    NTSEG1=10, NTSEG2=3, NSJW=3, NBRP=5 → 75% sample point
    CAN0->NBTP.reg =
        CAN_NBTP_NBRP(5)
        | CAN_NBTP_NTSEG1(10)
        | CAN_NBTP_NTSEG2(3)
        | CAN_NBTP_NSJW(3);

    // 4. Message RAM configuration (word addresses relative to SRAM base)
    // Standard ID filters (8 elements @ offset 0x00 words from CAN0_RAM_BASE)
    CAN0->SIDFC.reg =
        CAN_SIDFC_FLSSA(CAN0_RAM_BASE >> 2)
        | CAN_SIDFC_LSS(8);

    // Rx FIFO 0 (16 elements @ word offset 0x10)
    CAN0->RXF0C.reg =
        CAN_RXF0C_F0SA((CAN0_RAM_BASE + 0x10*4) >> 2)
        | CAN_RXF0C_F0S(16);

    // Rx element size: 8-byte data field
    CAN0->RXESC.reg = CAN_RXESC_F0DS(0x0) | CAN_RXESC_RBDS(0x0);

    // Tx Buffers (4 dedicated, no FIFO @ word offset 0x50)
    CAN0->TXBC.reg =
        CAN_TXBC_TBSA((CAN0_RAM_BASE + 0x50*4) >> 2)
        | CAN_TXBC_NDTB(4);

    // Tx element size: 8-byte data field
    CAN0->TXESC.reg = CAN_TXESC_TBDS(0x0);

    // 5. Accept all messages (no filters active → all go to Rx FIFO 0)
    CAN0->GFC.reg = CAN_GFC_ANFS(0x0) | CAN_GFC_ANFE(0x0);

    // 6. Enable interrupt LINE0 for Rx FIFO 0 New Message
    CAN0->IR.reg = 0xFFFFFFFF;  // clear all
    CAN0->IE.reg = CAN_IE_RF0NE;
    CAN0->ILE.reg = CAN_ILE_EINT0;
    NVIC_EnableIRQ(CAN0_IRQn);

    // 7. Start normal operation: clear CCE, then INIT
    CAN0->CCCR.reg &= ~CAN_CCCR_CCE;
    CAN0->CCCR.reg &= ~CAN_CCCR_INIT;
    // Wait until INIT reads 0 (write-synchronized)
    while (CAN0->CCCR.reg & CAN_CCCR_INIT);
}
```

## Transmit — Classic CAN Frame (Dedicated Tx Buffer 0)

```cpp
bool can0_send(uint32_t id, bool ext, const uint8_t *data, uint8_t dlc) {
    // Check buffer 0 not pending
    if (CAN0->TXBRP.reg & (1u << 0)) return false;

    // Calculate address of Tx buffer 0 in Message RAM
    // Tx element = T0 word + T1 word + 2 data words (for 8 bytes = 4 words total)
    uint32_t *elem = (uint32_t *)(CAN0_RAM_BASE + 0x50*4);

    // T0: ID + frame type
    if (ext) {
        elem[0] = CAN_TX_ELEMENT_T0_ID(id) | CAN_TX_ELEMENT_T0_XTD;
    } else {
        elem[0] = CAN_TX_ELEMENT_T0_ID(id << 18);  // 11-bit ID in bits 28:18
    }
    elem[1] = CAN_TX_ELEMENT_T1_DLC(dlc);

    // Data
    memcpy(&elem[2], data, dlc);

    // Request transmission of buffer 0
    CAN0->TXBAR.reg = (1u << 0);
    return true;
}
```

## Receive — Poll Rx FIFO 0

```cpp
bool can0_recv(uint32_t *id, bool *ext, uint8_t *data, uint8_t *dlc) {
    if (!(CAN0->RXF0S.reg & CAN_RXF0S_F0FL_Msk)) return false;  // empty

    uint32_t gi = (CAN0->RXF0S.reg & CAN_RXF0S_F0GI_Msk) >> CAN_RXF0S_F0GI_Pos;
    // Rx element = 4 words at (F0SA + gi * 4) words
    uint32_t *elem = (uint32_t *)(CAN0_RAM_BASE + 0x10*4) + gi * 4;

    *ext = (elem[0] >> 30) & 1;  // XTD bit
    if (*ext) {
        *id = elem[0] & 0x1FFFFFFF;
    } else {
        *id = (elem[0] >> 18) & 0x7FF;  // 11-bit ID
    }
    *dlc = (elem[1] >> 16) & 0xF;
    memcpy(data, &elem[2], *dlc);

    // Acknowledge (release element)
    CAN0->RXF0A.reg = CAN_RXF0A_F0AI(gi);
    return true;
}
```

## CAN FD Enable

```cpp
// Add after CCE=1, before clearing INIT:
CAN0->CCCR.reg |= CAN_CCCR_FDOE;     // Enable FD frames
CAN0->CCCR.reg |= CAN_CCCR_BRSE;     // Enable bit rate switching

// Data phase bit timing (e.g., 2 Mbps data @ 48 MHz)
// t_q = (2+1)/48MHz = 62.5ns, 8 t_q/bit = 500ns → 2Mbps
CAN0->DBTP.reg =
    CAN_DBTP_DBRP(2)
    | CAN_DBTP_DTSEG1(5)
    | CAN_DBTP_DTSEG2(1)
    | CAN_DBTP_DSJW(1)
    | CAN_DBTP_TDC;     // enable transceiver delay compensation

// TDCR: delay compensation offset (depends on transceiver, typically 5-10 tq)
CAN0->TDCR.reg = CAN_TDCR_TDCO(5);
```

## Sleep / Low-Power Sequence

```cpp
// Enter low-power:
CAN0->CCCR.reg |= CAN_CCCR_CSR;
while (!(CAN0->CCCR.reg & CAN_CCCR_CSA));  // wait acknowledgment
// Now safe to stop CLK_CAN_AHB and GCLK_CAN

// Wake up:
// Re-enable clocks first, then:
CAN0->CCCR.reg &= ~CAN_CCCR_CSR;
while (CAN0->CCCR.reg & CAN_CCCR_CSA);     // wait deacknowledgment
CAN0->CCCR.reg &= ~CAN_CCCR_INIT;          // resume normal operation
```

## Bus Monitoring Mode (Receive-Only)

```cpp
// After CCE=1+INIT=1:
CAN0->CCCR.reg |= CAN_CCCR_MON;
// CAN will receive but not transmit any dominant bits
// Clear INIT to start receiving
```

## Rx FIFO 0 ISR

```cpp
void CAN0_Handler(void) {
    uint32_t ir = CAN0->IR.reg;

    if (ir & CAN_IR_RF0N) {
        CAN0->IR.reg = CAN_IR_RF0N;  // clear flag
        // Process all elements in FIFO
        while ((CAN0->RXF0S.reg & CAN_RXF0S_F0FL_Msk) > 0) {
            uint8_t buf[8];
            uint32_t id;
            bool ext;
            uint8_t dlc;
            can0_recv(&id, &ext, buf, &dlc);
            app_on_can_frame(id, ext, buf, dlc);
        }
    }
    if (ir & CAN_IR_BO) {
        CAN0->IR.reg = CAN_IR_BO;
        // Bus-Off recovery: hardware auto-sets CCCR.INIT
        // Application must clear INIT after 128×11 recessive bits
    }
}
```

## Bit Timing Reference

```
f_CAN = f_GCLK / (NBRP+1) / (NTSEG1 + NTSEG2 + 3)
```

| f_GCLK_CAN | f_CAN | NBRP | NTSEG1 | NTSEG2 | NSJW |
|-----------|-------|------|--------|--------|------|
| 8 MHz | 500 kbps | 0 | 10 | 3 | 6 |
| 48 MHz | 500 kbps | 5 | 10 | 3 | 3 |
| 48 MHz | 250 kbps | 11 | 10 | 3 | 3 |
| 48 MHz | 1 Mbps | 2 | 10 | 4 | 4 |

## Key Facts

- CAN present on SAM C21 only (not SAM C20).
- CCCR.INIT=1 at reset — must clear it to start. Also auto-set on Bus-Off.
- Write access to configuration registers: CCCR.INIT=1 AND CCCR.CCE=1.
- TXBAR/TXBCR are writable only while CCCR.CCE=0 (normal operation).
- Message RAM addresses in configuration registers are word addresses (>> 2 from byte address).
- FIFO element address = base_addr + get_index × element_size_words × 4.
- After reading an Rx FIFO element, write get index to RXFnA to release it.
- Bus-Off recovery: CCCR.INIT is auto-set; application clears it after recovery pause.
- CAN FD: set FDOE + BRSE in CCCR; set FDF+BRS bits in each Tx buffer element to use fast data phase.
- Transceiver delay compensation (TDC): needed at data rates above ~2 Mbps; set DBTP.TDC=1 and configure TDCR.TDCO.

## See Also

- [[SAMC21 Datasheet Ch.34 CAN]] — full register reference, Message RAM elements, filter element formats
- [[I/O Multiplexing]] — CAN0/CAN1 TX/RX pin assignments
- [[Clock System]] — GCLK_CAN setup, AHBMASK enable
- [[NVIC Interrupt Map]] — CAN0/CAN1 IRQn values
