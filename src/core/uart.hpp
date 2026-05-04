#ifndef _UART_HPP_
#define _UART_HPP_

#include <stddef.h>
#include <stdint.h>
#include "samc21.h"
#include "pin.hpp"

enum class UartMode : uint8_t
{
  Interrupt = 0,
  Dma = 1,
};

template <uint8_t index>
struct SercomTraits;

template <>
struct SercomTraits<0>
{
  static inline Sercom* sercom(void) { return SERCOM0; }
  static constexpr IRQn_Type irqn = SERCOM0_IRQn;
  static constexpr uint32_t apb_mask = MCLK_APBCMASK_SERCOM0;
  static constexpr uint8_t gclk_id_core = SERCOM0_GCLK_ID_CORE;
  static constexpr uint8_t dmac_id_rx = SERCOM0_DMAC_ID_RX;
  static constexpr uint8_t dmac_id_tx = SERCOM0_DMAC_ID_TX;
};

template <>
struct SercomTraits<1>
{
  static inline Sercom* sercom(void) { return SERCOM1; }
  static constexpr IRQn_Type irqn = SERCOM1_IRQn;
  static constexpr uint32_t apb_mask = MCLK_APBCMASK_SERCOM1;
  static constexpr uint8_t gclk_id_core = SERCOM1_GCLK_ID_CORE;
  static constexpr uint8_t dmac_id_rx = SERCOM1_DMAC_ID_RX;
  static constexpr uint8_t dmac_id_tx = SERCOM1_DMAC_ID_TX;
};

template <>
struct SercomTraits<2>
{
  static inline Sercom* sercom(void) { return SERCOM2; }
  static constexpr IRQn_Type irqn = SERCOM2_IRQn;
  static constexpr uint32_t apb_mask = MCLK_APBCMASK_SERCOM2;
  static constexpr uint8_t gclk_id_core = SERCOM2_GCLK_ID_CORE;
  static constexpr uint8_t dmac_id_rx = SERCOM2_DMAC_ID_RX;
  static constexpr uint8_t dmac_id_tx = SERCOM2_DMAC_ID_TX;
};

template <>
struct SercomTraits<3>
{
  static inline Sercom* sercom(void) { return SERCOM3; }
  static constexpr IRQn_Type irqn = SERCOM3_IRQn;
  static constexpr uint32_t apb_mask = MCLK_APBCMASK_SERCOM3;
  static constexpr uint8_t gclk_id_core = SERCOM3_GCLK_ID_CORE;
  static constexpr uint8_t dmac_id_rx = SERCOM3_DMAC_ID_RX;
  static constexpr uint8_t dmac_id_tx = SERCOM3_DMAC_ID_TX;
};

template <>
struct SercomTraits<4>
{
  static inline Sercom* sercom(void) { return SERCOM4; }
  static constexpr IRQn_Type irqn = SERCOM4_IRQn;
  static constexpr uint32_t apb_mask = MCLK_APBCMASK_SERCOM4;
  static constexpr uint8_t gclk_id_core = SERCOM4_GCLK_ID_CORE;
  static constexpr uint8_t dmac_id_rx = SERCOM4_DMAC_ID_RX;
  static constexpr uint8_t dmac_id_tx = SERCOM4_DMAC_ID_TX;
};

template <>
struct SercomTraits<5>
{
  static inline Sercom* sercom(void) { return SERCOM5; }
  static constexpr IRQn_Type irqn = SERCOM5_IRQn;
  static constexpr uint32_t apb_mask = MCLK_APBCMASK_SERCOM5;
  static constexpr uint8_t gclk_id_core = SERCOM5_GCLK_ID_CORE;
  static constexpr uint8_t dmac_id_rx = SERCOM5_DMAC_ID_RX;
  static constexpr uint8_t dmac_id_tx = SERCOM5_DMAC_ID_TX;
};

template <
    typename TxPinT,
    typename RxPinT,
    sam::gpio::Peripheral peripheral_mux,
    uint8_t txpo_value,
    uint8_t rxpo_value>
struct UartPinout
{
  typedef TxPinT TxPin;
  typedef RxPinT RxPin;

  static constexpr uint8_t txpo = txpo_value;
  static constexpr uint8_t rxpo = rxpo_value;

  static inline sam::gpio::Peripheral peripheral(void)
  {
    return peripheral_mux;
  }

  static inline void apply(void)
  {
    // Keep UART TX at idle level while switching pin mux to avoid spurious bytes.
    TxPin::as_output(true);
    TxPin::set_peripheral(peripheral());
    RxPin::as_input();
    RxPin::set_peripheral(peripheral());
  }
};

template <UartMode mode, typename Traits, typename Pinout, size_t RxN = 8, size_t TxN = 8>
class Uart;

template <typename Traits, typename Pinout, size_t RxN = 8, size_t TxN = 8>
using UartINT = Uart<UartMode::Interrupt, Traits, Pinout, RxN, TxN>;

template <typename Traits, typename Pinout, size_t RxN = 8, size_t TxN = 8>
using UartDMA = Uart<UartMode::Dma, Traits, Pinout, RxN, TxN>;

#include "uart_int.hpp"
#include "uart_dma.hpp"

#endif // _UART_HPP_
