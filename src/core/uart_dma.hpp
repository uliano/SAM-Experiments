#ifndef _UART_DMA_HPP_
#define _UART_DMA_HPP_

#include <stddef.h>
#include <stdint.h>

#include "ring_buffer.hpp"

template <typename Traits, typename Pinout, size_t RxN, size_t TxN>
class Uart<UartMode::Dma, Traits, Pinout, RxN, TxN>
{
  static_assert(Pinout::txpo < 4, "TXPO must be in range 0..3");
  static_assert(Pinout::rxpo < 4, "RXPO must be in range 0..3");
  static_assert(DMAC_CH_NUM >= 2, "DMAC must expose at least two channels");

public:
  struct Stats
  {
    uint32_t rx_overwrite_count;
    uint32_t tx_drop_count;
    uint32_t parity_error_count;
    uint32_t frame_error_count;
    uint32_t hw_overflow_count;
  };

  Uart(void) :
    initialized_(false),
    tx_dma_active_(false),
    rx_dma_last_total_(0),
    rx_dma_block_count_(0)
  {
    clear_stats();
  }

  void init(uint32_t baud, uint8_t gclk_generator = 0)
  {
    uint32_t irq_state = irq_lock();
    initialized_ = false;
    tx_dma_active_ = false;
    rx_dma_last_total_ = 0;
    rx_dma_block_count_ = 0;
    rx_.clear();
    tx_.clear();
    clear_stats_unsafe();
    irq_unlock(irq_state);

    Pinout::apply();

    MCLK->APBCMASK.reg |= Traits::apb_mask;
    GCLK->PCHCTRL[Traits::gclk_id_core].reg = GCLK_PCHCTRL_GEN(gclk_generator) | GCLK_PCHCTRL_CHEN;
    while (0 == (GCLK->PCHCTRL[Traits::gclk_id_core].reg & GCLK_PCHCTRL_CHEN));

    SercomUsart &u = usart();

    u.CTRLA.reg = SERCOM_USART_CTRLA_SWRST;
    while (u.SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_SWRST);
    while (u.CTRLA.reg & SERCOM_USART_CTRLA_SWRST);

    const uint64_t br = (uint64_t)65536u * (F_CPU - 16u * baud) / F_CPU;

    u.CTRLA.reg =
        SERCOM_USART_CTRLA_DORD |
        SERCOM_USART_CTRLA_MODE(1/*USART_INT_CLK*/) |
        SERCOM_USART_CTRLA_RXPO(Pinout::rxpo) |
        SERCOM_USART_CTRLA_TXPO(Pinout::txpo);

    u.CTRLB.reg =
        SERCOM_USART_CTRLB_RXEN |
        SERCOM_USART_CTRLB_TXEN |
        SERCOM_USART_CTRLB_CHSIZE(0/*8 bits*/);
    while (u.SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_CTRLB);

    u.BAUD.reg = (uint16_t)br;

    u.INTENCLR.reg = 0xff;
    u.INTFLAG.reg = 0xff;
    u.STATUS.reg = SERCOM_USART_STATUS_PERR | SERCOM_USART_STATUS_FERR | SERCOM_USART_STATUS_BUFOVF;

    dmac_init_once();
    dmac_init_channel(rx_dma_channel(), Traits::dmac_id_rx, true);
    dmac_init_channel(tx_dma_channel(), Traits::dmac_id_tx, false);

    irq_state = irq_lock();
    start_rx_dma_unsafe();
    irq_unlock(irq_state);

    u.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
    while (u.SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_ENABLE);

    u.INTENSET.reg = SERCOM_USART_INTENSET_ERROR;

    NVIC_ClearPendingIRQ(Traits::irqn);
    NVIC_EnableIRQ(Traits::irqn);

    initialized_ = true;
  }

  bool write(uint8_t value)
  {
    if (!initialized_)
      return false;

    uint32_t irq_state = irq_lock();

    if (tx_.full())
    {
      ++stats_.tx_drop_count;
      irq_unlock(irq_state);
      return false;
    }

    tx_.push(value);
    if (!tx_dma_active_)
      start_tx_dma_unsafe();

    irq_unlock(irq_state);
    return true;
  }

  size_t write(const uint8_t *data, size_t size)
  {
    size_t written = 0;

    while (written < size)
    {
      if (!write(data[written]))
        break;
      ++written;
    }

    return written;
  }

  size_t write(const char *text)
  {
    size_t written = 0;

    while (*text)
    {
      if (!write((uint8_t)*text++))
        break;
      ++written;
    }

    return written;
  }

  void write_blocking(uint8_t value)
  {
    while (!write(value));
  }

  void write_blocking(const char *text)
  {
    while (*text)
      write_blocking((uint8_t)*text++);
  }

  bool read(uint8_t& out)
  {
    if (!initialized_)
      return false;

    pump_rx_from_dma();

    uint32_t irq_state = irq_lock();
    bool ok = rx_.pop(out);
    irq_unlock(irq_state);
    return ok;
  }

  size_t available(void)
  {
    if (!initialized_)
      return 0;

    pump_rx_from_dma();

    uint32_t irq_state = irq_lock();
    size_t n = rx_.size();
    irq_unlock(irq_state);
    return n;
  }

  Stats get_stats(void) const
  {
    uint32_t irq_state = irq_lock();
    Stats copy = stats_;
    irq_unlock(irq_state);
    return copy;
  }

  void clear_stats(void)
  {
    uint32_t irq_state = irq_lock();
    clear_stats_unsafe();
    irq_unlock(irq_state);
  }

  bool tx_idle(void) const
  {
    uint32_t irq_state = irq_lock();
    bool idle = tx_.empty() &&
        !tx_dma_active_ &&
        (usart().INTFLAG.reg & SERCOM_USART_INTFLAG_DRE);
    irq_unlock(irq_state);
    return idle;
  }

  bool is_initialized(void) const
  {
    return initialized_;
  }

  void irq_handler(void)
  {
    if (!initialized_)
      return;

    SercomUsart &u = usart();
    const uint8_t flags = u.INTFLAG.reg;

    if (flags & SERCOM_USART_INTFLAG_ERROR)
    {
      const uint16_t status = u.STATUS.reg;

      if (status & SERCOM_USART_STATUS_PERR)
        ++stats_.parity_error_count;

      if (status & SERCOM_USART_STATUS_FERR)
        ++stats_.frame_error_count;

      if (status & SERCOM_USART_STATUS_BUFOVF)
        ++stats_.hw_overflow_count;

      u.STATUS.reg = status & (SERCOM_USART_STATUS_PERR | SERCOM_USART_STATUS_FERR | SERCOM_USART_STATUS_BUFOVF);
      u.INTFLAG.reg = SERCOM_USART_INTFLAG_ERROR;
    }
  }

  void dma_irq_handler(void)
  {
    if (!initialized_)
      return;

    while (DMAC->INTSTATUS.reg)
    {
      const uint8_t channel = DMAC->INTPEND.bit.ID;
      dmac_select_channel(channel);

      const uint8_t flags = DMAC->CHINTFLAG.reg;
      DMAC->CHINTFLAG.reg = flags;

      if (channel == rx_dma_channel())
      {
        if (flags & DMAC_CHINTFLAG_TCMPL)
          rx_dma_block_count_ = rx_dma_block_count_ + 1u;

        if (flags & DMAC_CHINTFLAG_TERR)
        {
          ++stats_.hw_overflow_count;
          const uint32_t irq_state = irq_lock();
          start_rx_dma_unsafe();
          irq_unlock(irq_state);
        }
      }
      else if (channel == tx_dma_channel())
      {
        if (flags & DMAC_CHINTFLAG_TERR)
          ++stats_.tx_drop_count;

        if ((flags & DMAC_CHINTFLAG_TCMPL) || (flags & DMAC_CHINTFLAG_TERR))
        {
          const uint32_t irq_state = irq_lock();
          tx_dma_active_ = false;
          start_tx_dma_unsafe();
          irq_unlock(irq_state);
        }
      }
    }
  }

private:
  static constexpr uint8_t RX_DMA_CHANNEL = 0;
  static constexpr uint8_t TX_DMA_CHANNEL = 1;
  static constexpr size_t RX_DMA_BUFFER_SIZE = 128;
  static constexpr size_t TX_DMA_CHUNK_SIZE = 64;

  static_assert(RX_DMA_CHANNEL < DMAC_CH_NUM, "Invalid RX DMA channel");
  static_assert(TX_DMA_CHANNEL < DMAC_CH_NUM, "Invalid TX DMA channel");
  static_assert(RX_DMA_CHANNEL != TX_DMA_CHANNEL, "RX/TX DMA channels must differ");

  static inline SercomUsart& usart(void)
  {
    return Traits::sercom()->USART;
  }

  static inline uint32_t irq_lock(void)
  {
    const uint32_t state = __get_PRIMASK();
    __disable_irq();
    return state;
  }

  static inline void irq_unlock(uint32_t state)
  {
    if (0 == (state & 1u))
      __enable_irq();
  }

  static constexpr uint8_t rx_dma_channel(void)
  {
    return RX_DMA_CHANNEL;
  }

  static constexpr uint8_t tx_dma_channel(void)
  {
    return TX_DMA_CHANNEL;
  }

  static void dmac_init_once(void)
  {
    if (dmac_initialized_)
      return;

    MCLK->AHBMASK.reg |= MCLK_AHBMASK_DMAC;

    DMAC->CTRL.reg = DMAC_CTRL_SWRST;
    while (DMAC->CTRL.reg & DMAC_CTRL_SWRST);

    for (size_t i = 0; i < DMAC_CH_NUM; ++i)
    {
      dma_descriptor_[i].BTCTRL.reg = 0;
      dma_descriptor_[i].BTCNT.reg = 0;
      dma_descriptor_[i].SRCADDR.reg = 0;
      dma_descriptor_[i].DSTADDR.reg = 0;
      dma_descriptor_[i].DESCADDR.reg = 0;

      dma_writeback_[i].BTCTRL.reg = 0;
      dma_writeback_[i].BTCNT.reg = 0;
      dma_writeback_[i].SRCADDR.reg = 0;
      dma_writeback_[i].DSTADDR.reg = 0;
      dma_writeback_[i].DESCADDR.reg = 0;
    }

    DMAC->BASEADDR.reg = (uint32_t)(uintptr_t)&dma_descriptor_[0];
    DMAC->WRBADDR.reg = (uint32_t)(uintptr_t)&dma_writeback_[0];
    DMAC->CTRL.reg = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN0;

    NVIC_ClearPendingIRQ(DMAC_IRQn);
    NVIC_EnableIRQ(DMAC_IRQn);

    dmac_initialized_ = true;
  }

  static void dmac_select_channel(uint8_t channel)
  {
    DMAC->CHID.reg = DMAC_CHID_ID(channel);
  }

  static void dmac_init_channel(uint8_t channel, uint8_t trigsrc, bool enable_channel_interrupt)
  {
    dmac_select_channel(channel);

    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;
    while (DMAC->CHCTRLA.reg & DMAC_CHCTRLA_SWRST);

    DMAC->CHCTRLB.reg =
        DMAC_CHCTRLB_LVL(0) |
        DMAC_CHCTRLB_TRIGSRC(trigsrc) |
        DMAC_CHCTRLB_TRIGACT_BEAT;

    DMAC->CHINTENCLR.reg = 0xff;
    DMAC->CHINTFLAG.reg = DMAC_CHINTFLAG_TERR | DMAC_CHINTFLAG_TCMPL | DMAC_CHINTFLAG_SUSP;

    if (enable_channel_interrupt)
      DMAC->CHINTENSET.reg = DMAC_CHINTENSET_TERR | DMAC_CHINTENSET_TCMPL;
    else
      DMAC->CHINTENSET.reg = DMAC_CHINTENSET_TERR | DMAC_CHINTENSET_TCMPL;
  }

  void start_rx_dma_unsafe(void)
  {
    dmac_select_channel(rx_dma_channel());
    DMAC->CHCTRLA.reg &= (uint8_t)~DMAC_CHCTRLA_ENABLE;
    while (DMAC->CHCTRLA.reg & DMAC_CHCTRLA_ENABLE);

    dma_descriptor_[rx_dma_channel()].BTCTRL.reg =
        DMAC_BTCTRL_VALID |
        DMAC_BTCTRL_BEATSIZE_BYTE |
        DMAC_BTCTRL_DSTINC;
    dma_descriptor_[rx_dma_channel()].BTCNT.reg = RX_DMA_BUFFER_SIZE;
    dma_descriptor_[rx_dma_channel()].SRCADDR.reg = (uint32_t)(uintptr_t)&usart().DATA.reg;
    dma_descriptor_[rx_dma_channel()].DSTADDR.reg = (uint32_t)(uintptr_t)(rx_dma_buffer_ + RX_DMA_BUFFER_SIZE);
    dma_descriptor_[rx_dma_channel()].DESCADDR.reg = (uint32_t)(uintptr_t)&dma_descriptor_[rx_dma_channel()];

    dma_writeback_[rx_dma_channel()].BTCTRL.reg = 0;
    dma_writeback_[rx_dma_channel()].BTCNT.reg = 0;
    dma_writeback_[rx_dma_channel()].SRCADDR.reg = 0;
    dma_writeback_[rx_dma_channel()].DSTADDR.reg = 0;
    dma_writeback_[rx_dma_channel()].DESCADDR.reg = 0;

    rx_dma_last_total_ = 0;
    rx_dma_block_count_ = 0;

    DMAC->CHINTFLAG.reg = DMAC_CHINTFLAG_TERR | DMAC_CHINTFLAG_TCMPL | DMAC_CHINTFLAG_SUSP;
    __DMB();
    DMAC->CHCTRLA.reg |= DMAC_CHCTRLA_ENABLE;
  }

  bool start_tx_dma_unsafe(void)
  {
    if (tx_dma_active_ || tx_.empty())
      return false;

    size_t count = 0;
    while ((count < TX_DMA_CHUNK_SIZE) && !tx_.empty())
    {
      tx_.pop(tx_dma_chunk_[count]);
      ++count;
    }

    if (0 == count)
      return false;

    dmac_select_channel(tx_dma_channel());
    DMAC->CHCTRLA.reg &= (uint8_t)~DMAC_CHCTRLA_ENABLE;
    while (DMAC->CHCTRLA.reg & DMAC_CHCTRLA_ENABLE);

    dma_descriptor_[tx_dma_channel()].BTCTRL.reg =
        DMAC_BTCTRL_VALID |
        DMAC_BTCTRL_BEATSIZE_BYTE |
        DMAC_BTCTRL_SRCINC;
    dma_descriptor_[tx_dma_channel()].BTCNT.reg = (uint16_t)count;
    dma_descriptor_[tx_dma_channel()].SRCADDR.reg = (uint32_t)(uintptr_t)(tx_dma_chunk_ + count);
    dma_descriptor_[tx_dma_channel()].DSTADDR.reg = (uint32_t)(uintptr_t)&usart().DATA.reg;
    dma_descriptor_[tx_dma_channel()].DESCADDR.reg = 0;

    DMAC->CHINTFLAG.reg = DMAC_CHINTFLAG_TERR | DMAC_CHINTFLAG_TCMPL | DMAC_CHINTFLAG_SUSP;
    __DMB();
    DMAC->CHCTRLA.reg |= DMAC_CHCTRLA_ENABLE;

    tx_dma_active_ = true;
    return true;
  }

  void snapshot_rx_position(uint32_t& block_count, uint16_t& remaining)
  {
    while (1)
    {
      const uint32_t irq_state = irq_lock();
      const uint32_t b0 = rx_dma_block_count_;
      dmac_select_channel(rx_dma_channel());
      bool suspended = false;

      if (DMAC->CHCTRLA.reg & DMAC_CHCTRLA_ENABLE)
      {
        uint32_t chctrlb = DMAC->CHCTRLB.reg;
        chctrlb &= ~DMAC_CHCTRLB_CMD_Msk;
        DMAC->CHCTRLB.reg = chctrlb | DMAC_CHCTRLB_CMD_SUSPEND;

        for (uint16_t spin = 0; spin < 512u; ++spin)
        {
          if (DMAC->CHINTFLAG.reg & DMAC_CHINTFLAG_SUSP)
          {
            suspended = true;
            break;
          }
        }
      }

      const uint16_t r = dma_writeback_[rx_dma_channel()].BTCNT.reg;

      if (suspended)
      {
        DMAC->CHINTFLAG.reg = DMAC_CHINTFLAG_SUSP;

        uint32_t chctrlb = DMAC->CHCTRLB.reg;
        chctrlb &= ~DMAC_CHCTRLB_CMD_Msk;
        DMAC->CHCTRLB.reg = chctrlb | DMAC_CHCTRLB_CMD_RESUME;
      }

      const uint32_t b1 = rx_dma_block_count_;
      irq_unlock(irq_state);

      if (b0 == b1)
      {
        block_count = b0;
        remaining = r;
        return;
      }
    }
  }

  void pump_rx_from_dma(void)
  {
    uint32_t block_count = 0;
    uint16_t remaining = 0;
    snapshot_rx_position(block_count, remaining);

    size_t write_index = RX_DMA_BUFFER_SIZE - (size_t)remaining;
    if (write_index >= RX_DMA_BUFFER_SIZE)
      write_index = 0;

    const uint32_t produced_total = block_count * (uint32_t)RX_DMA_BUFFER_SIZE + (uint32_t)write_index;
    size_t pending = (size_t)(produced_total - rx_dma_last_total_);

    if (pending > RX_DMA_BUFFER_SIZE)
    {
      stats_.rx_overwrite_count += (pending - RX_DMA_BUFFER_SIZE);
      rx_dma_last_total_ = produced_total - RX_DMA_BUFFER_SIZE;
      pending = RX_DMA_BUFFER_SIZE;
    }

    while (pending > 0)
    {
      const size_t index = (size_t)(rx_dma_last_total_ % RX_DMA_BUFFER_SIZE);
      if (rx_.push(rx_dma_buffer_[index]))
        ++stats_.rx_overwrite_count;

      ++rx_dma_last_total_;
      --pending;
    }
  }

  void clear_stats_unsafe(void)
  {
    stats_.rx_overwrite_count = 0;
    stats_.tx_drop_count = 0;
    stats_.parity_error_count = 0;
    stats_.frame_error_count = 0;
    stats_.hw_overflow_count = 0;
  }

  RingBuffer<uint8_t, RxN> rx_;
  RingBuffer<uint8_t, TxN> tx_;
  Stats stats_;
  bool initialized_;
  bool tx_dma_active_;

  uint8_t tx_dma_chunk_[TX_DMA_CHUNK_SIZE];
  uint8_t rx_dma_buffer_[RX_DMA_BUFFER_SIZE];
  uint32_t rx_dma_last_total_;
  volatile uint32_t rx_dma_block_count_;

  alignas(16) static inline volatile DmacDescriptor dma_descriptor_[DMAC_CH_NUM];
  alignas(16) static inline volatile DmacDescriptor dma_writeback_[DMAC_CH_NUM];
  static inline bool dmac_initialized_ = false;
};

#endif // _UART_DMA_HPP_
