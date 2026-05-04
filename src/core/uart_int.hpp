#ifndef _UART_INT_HPP_
#define _UART_INT_HPP_

#include <stddef.h>
#include <stdint.h>
#include "ring_buffer.hpp"

template <typename Traits, typename Pinout, size_t RxN, size_t TxN>
class Uart<UartMode::Interrupt, Traits, Pinout, RxN, TxN>
{
  static_assert(Pinout::txpo < 4, "TXPO must be in range 0..3");
  static_assert(Pinout::rxpo < 4, "RXPO must be in range 0..3");

public:
  struct Stats
  {
    uint32_t rx_overwrite_count;
    uint32_t tx_drop_count;
    uint32_t parity_error_count;
    uint32_t frame_error_count;
    uint32_t hw_overflow_count;
  };

  Uart(void) : initialized_(false)
  {
    clear_stats();
  }

  void init(uint32_t baud, uint8_t gclk_generator = 0)
  {
    rx_.clear();
    tx_.clear();
    clear_stats();

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

    u.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
    while (u.SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_ENABLE);

    u.INTENSET.reg = SERCOM_USART_INTENSET_RXC | SERCOM_USART_INTENSET_ERROR;

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
    usart().INTENSET.reg = SERCOM_USART_INTENSET_DRE;

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
    uint32_t irq_state = irq_lock();
    bool ok = rx_.pop(out);
    irq_unlock(irq_state);
    return ok;
  }

  size_t available(void)
  {
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
    stats_.rx_overwrite_count = 0;
    stats_.tx_drop_count = 0;
    stats_.parity_error_count = 0;
    stats_.frame_error_count = 0;
    stats_.hw_overflow_count = 0;
    irq_unlock(irq_state);
  }

  bool tx_idle(void) const
  {
    uint32_t irq_state = irq_lock();
    bool idle = tx_.empty() && (usart().INTFLAG.reg & SERCOM_USART_INTFLAG_DRE);
    irq_unlock(irq_state);
    return idle;
  }

  bool is_initialized(void) const
  {
    return initialized_;
  }

  void irq_handler(void)
  {
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

    if (flags & SERCOM_USART_INTFLAG_RXC)
    {
      const uint8_t value = (uint8_t)u.DATA.reg;
      if (rx_.push(value))
        ++stats_.rx_overwrite_count;
    }

    if (flags & SERCOM_USART_INTFLAG_DRE)
    {
      uint8_t value = 0;

      if (tx_.pop(value))
      {
        u.DATA.reg = value;
      }
      else
      {
        u.INTENCLR.reg = SERCOM_USART_INTENCLR_DRE;
      }
    }
  }

  void dma_irq_handler(void)
  {
  }

private:
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

  RingBuffer<uint8_t, RxN> rx_;
  RingBuffer<uint8_t, TxN> tx_;
  Stats stats_;
  bool initialized_;
};

#endif // _UART_INT_HPP_
