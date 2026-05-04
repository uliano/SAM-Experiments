#ifndef _SERIAL_HPP_
#define _SERIAL_HPP_

#include "pins.hpp"
#include "serial_port.hpp"
#include "uart.hpp"

using SerialType = SerialPort<
  UartINT<SercomTraits<5>,
          UartPinout<UartTxPin, UartRxPin, sam::gpio::Peripheral::D, 0, 1>,
          5, 8>>;

extern SerialType Serial;

#endif // _SERIAL_HPP_
