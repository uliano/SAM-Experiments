#ifndef _SERIAL_PORT_HPP_
#define _SERIAL_PORT_HPP_

#include <stddef.h>
#include <stdint.h>

#include "bytestream.hpp"

template <typename UartT>
class SerialPort : public ByteStreamPrintMixin<SerialPort<UartT>>
{
public:
  SerialPort() = default;

  void init(uint32_t baud, uint8_t gclk = 0)
  {
    uart_.init(baud, gclk);
  }

  bool ready() const
  {
    return uart_.is_initialized();
  }

  // ByteStream interface
  size_t write(const uint8_t *data, size_t size)
  {
    return uart_.write(data, size);
  }

  size_t read(uint8_t *data, size_t size)
  {
    size_t count = 0;
    uint8_t value = 0;

    while (count < size)
    {
      if (!uart_.read(value))
        break;

      data[count++] = value;
    }

    return count;
  }

  size_t available(void)
  {
    return uart_.available();
  }

  void flush()
  {
    while (!uart_.tx_idle());
  }

  void irq_handler()
  {
    uart_.irq_handler();
  }

private:
  UartT uart_;
};

#endif // _SERIAL_PORT_HPP_
