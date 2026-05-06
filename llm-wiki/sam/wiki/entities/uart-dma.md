---
title: Uart DMA
type: entity
tags: [uart, sercom, dma, dmac, firmware, template]
sources: [firmware-core-library]
created: 2026-05-05
updated: 2026-05-05
---

# Uart DMA

DMA-driven UART over SERCOM USART + DMAC. Defined in `src/core/uart_dma.hpp` as a partial
specialization of `Uart<UartMode::Dma, Traits, Pinout, RxN, TxN>`.

## Template Parameters

Same as [[Uart INT]] but mode = `UartMode::Dma`.

Requires `DMAC_CH_NUM >= 2`.

## Fixed Constants

```cpp
static constexpr uint8_t  RX_DMA_CHANNEL   = 0;
static constexpr uint8_t  TX_DMA_CHANNEL   = 1;
static constexpr size_t   RX_DMA_BUFFER_SIZE = 128;   // hardware DMA ring
static constexpr size_t   TX_DMA_CHUNK_SIZE  = 64;    // max bytes per DMA TX transfer
```

## Static Members (shared across all instances)

```cpp
alignas(16) static inline volatile DmacDescriptor dma_descriptor_[DMAC_CH_NUM];
alignas(16) static inline volatile DmacDescriptor dma_writeback_[DMAC_CH_NUM];
static inline bool dmac_initialized_ = false;
```

DMAC descriptor and writeback tables must be 16-byte aligned — enforced at declaration.
`dmac_init_once()` is called from `init()` but only initializes DMAC once even if multiple
DMA UARTs exist.

## Initialization

```cpp
uart.init(baud, gclk_generator = 0);
```

Steps beyond [[Uart INT]] init:
1. `dmac_init_once()` — enables DMAC AHBMASK, SW reset, sets BASEADDR/WRBADDR, enables LVLEN0, enables DMAC IRQ
2. `dmac_init_channel(rx_ch, DMAC_ID_RX, interrupt=true)` — TRIGACT_BEAT from SERCOM RX
3. `dmac_init_channel(tx_ch, DMAC_ID_TX, interrupt=true)` — TRIGACT_BEAT from SERCOM TX
4. `start_rx_dma_unsafe()` — arms first 128-byte RX descriptor (circular, `DESCADDR` points to itself)
5. SERCOM enabled with only ERROR interrupt (no RXC/DRE — handled by DMA)

## RX Data Flow

```
SERCOM DATA → DMAC (beat per byte) → rx_dma_buffer_[128] (circular)
pump_rx_from_dma() → rx_ (RingBuffer)
```

`pump_rx_from_dma()` is called from `read()` and `available()`. It:
1. Calls `snapshot_rx_position()` to get `(block_count, remaining)` consistently
2. Computes `produced_total = block_count * 128 + (128 - remaining)`
3. Copies `produced_total - rx_dma_last_total_` bytes from `rx_dma_buffer_` into `rx_`

`snapshot_rx_position()` briefly **suspends** the RX DMA channel to read the writeback BTCNT
atomically, then resumes it. Uses a spin loop (max 512 iterations) to wait for SUSP flag.

When a full 128-byte block completes, `dma_irq_handler()` increments `rx_dma_block_count_` (volatile).

## TX Data Flow

```
tx_ (RingBuffer) → tx_dma_chunk_[64] → DMAC → SERCOM DATA
```

`write()` → pushes to `tx_` → if no TX DMA active, calls `start_tx_dma_unsafe()`.
`start_tx_dma_unsafe()` drains up to 64 bytes from `tx_` into `tx_dma_chunk_`, sets up
descriptor, enables TX DMA channel. On TCMPL or TERR, `dma_irq_handler()` calls
`start_tx_dma_unsafe()` again to process remaining bytes.

## IRQ Handlers

Two separate handlers must be wired:

```cpp
void irq_handler();      // SERCOM IRQ — handles only ERROR flag
void dma_irq_handler();  // DMAC IRQ — handles RX block completion and TX completion
```

## Compared to Uart INT

| Aspect | Uart INT | Uart DMA |
|--------|----------|----------|
| RX | ISR per byte | DMA fills 128-byte buffer; SW drains on read() |
| TX | ISR per byte | DMA sends 64-byte chunks |
| CPU load | Higher at high baud rates | Lower at high baud rates |
| Complexity | Simple | snapshot_rx_position() is subtle |
| DMAC channels | Not used | 2 channels consumed (0 + 1) |

## See Also

- [[Uart INT]] — simpler interrupt-driven alternative
- [[DMA Controller]] — DMAC concept and register overview
- [[RingBuffer]] — software staging buffer
- [[IRQ Critical Section]] — used throughout
- [[SERCOM UART Configuration]] — common hardware init
