#ifndef _BYTESTREAM_HPP_
#define _BYTESTREAM_HPP_

#include <charconv>
#include <concepts>
#include <stddef.h>
#include <stdint.h>
#include <type_traits>

template <typename StreamT>
concept ByteStream =
  requires(StreamT stream, const uint8_t *tx_data, uint8_t *rx_data, size_t size)
{
  { stream.write(tx_data, size) } -> std::same_as<size_t>;
  { stream.read(rx_data, size) } -> std::same_as<size_t>;
  { stream.available() } -> std::same_as<size_t>;
};

enum class PrintBase : uint8_t
{
  Bin = 2,
  Dec = 10,
  Hex = 16,
};

namespace bytestream_detail
{
static constexpr size_t PRINT_BUFFER_SIZE = 70;

inline void uppercase_hex(char *begin, char *end)
{
  while (begin < end)
  {
    if ((*begin >= 'a') && (*begin <= 'f'))
      *begin = (char)(*begin - ('a' - 'A'));
    ++begin;
  }
}

template <typename UIntT>
inline size_t format_unsigned(UIntT value, PrintBase base, char *buffer, size_t size)
{
  char *out = buffer;

  if (PrintBase::Hex == base)
  {
    *out++ = '0';
    *out++ = 'x';
  }
  else if (PrintBase::Bin == base)
  {
    *out++ = '0';
    *out++ = 'b';
  }

  auto result = std::to_chars(out, buffer + size, value, (int)base);
  if (result.ec != std::errc())
    return 0;

  if (PrintBase::Hex == base)
    uppercase_hex(out, result.ptr);

  return (size_t)(result.ptr - buffer);
}

template <typename SIntT>
inline size_t format_signed_decimal(SIntT value, char *buffer, size_t size)
{
  auto result = std::to_chars(buffer, buffer + size, value, 10);
  if (result.ec != std::errc())
    return 0;

  return (size_t)(result.ptr - buffer);
}

template <ByteStream StreamT>
inline size_t write_chars(StreamT& stream, const char *data, size_t size)
{
  return stream.write((const uint8_t *)data, size);
}
}

template <ByteStream StreamT>
inline size_t bytestream_write_cstr(StreamT& stream, const char *text)
{
  if (0 == text)
    return 0;

  size_t count = 0;

  while (*text)
  {
    const uint8_t c = (uint8_t)*text++;
    if (1 != stream.write(&c, 1))
      break;
    ++count;
  }

  return count;
}

template <ByteStream StreamT>
inline size_t bytestream_print(StreamT& stream, const char *text)
{
  return bytestream_write_cstr(stream, text);
}

template <ByteStream StreamT>
inline size_t bytestream_print(StreamT& stream, uint8_t value, PrintBase base = PrintBase::Dec)
{
  char buffer[bytestream_detail::PRINT_BUFFER_SIZE];
  size_t size = bytestream_detail::format_unsigned((unsigned int)value, base, buffer, sizeof(buffer));
  return bytestream_detail::write_chars(stream, buffer, size);
}

template <ByteStream StreamT>
inline size_t bytestream_print(StreamT& stream, uint16_t value, PrintBase base = PrintBase::Dec)
{
  char buffer[bytestream_detail::PRINT_BUFFER_SIZE];
  size_t size = bytestream_detail::format_unsigned((unsigned int)value, base, buffer, sizeof(buffer));
  return bytestream_detail::write_chars(stream, buffer, size);
}

template <ByteStream StreamT>
inline size_t bytestream_print(StreamT& stream, uint32_t value, PrintBase base = PrintBase::Dec)
{
  char buffer[bytestream_detail::PRINT_BUFFER_SIZE];
  size_t size = bytestream_detail::format_unsigned(value, base, buffer, sizeof(buffer));
  return bytestream_detail::write_chars(stream, buffer, size);
}

template <ByteStream StreamT>
inline size_t bytestream_print(StreamT& stream, uint64_t value, PrintBase base = PrintBase::Dec)
{
  char buffer[bytestream_detail::PRINT_BUFFER_SIZE];
  size_t size = bytestream_detail::format_unsigned(value, base, buffer, sizeof(buffer));
  return bytestream_detail::write_chars(stream, buffer, size);
}

template <ByteStream StreamT>
inline size_t bytestream_print(StreamT& stream, int8_t value, PrintBase base = PrintBase::Dec)
{
  char buffer[bytestream_detail::PRINT_BUFFER_SIZE];
  size_t size = 0;

  if (PrintBase::Dec == base)
    size = bytestream_detail::format_signed_decimal((int)value, buffer, sizeof(buffer));
  else
    size = bytestream_detail::format_unsigned((unsigned int)(uint8_t)value, base, buffer, sizeof(buffer));

  return bytestream_detail::write_chars(stream, buffer, size);
}

template <ByteStream StreamT>
inline size_t bytestream_print(StreamT& stream, int16_t value, PrintBase base = PrintBase::Dec)
{
  char buffer[bytestream_detail::PRINT_BUFFER_SIZE];
  size_t size = 0;

  if (PrintBase::Dec == base)
    size = bytestream_detail::format_signed_decimal((int)value, buffer, sizeof(buffer));
  else
    size = bytestream_detail::format_unsigned((unsigned int)(uint16_t)value, base, buffer, sizeof(buffer));

  return bytestream_detail::write_chars(stream, buffer, size);
}

template <ByteStream StreamT>
inline size_t bytestream_print(StreamT& stream, int32_t value, PrintBase base = PrintBase::Dec)
{
  char buffer[bytestream_detail::PRINT_BUFFER_SIZE];
  size_t size = 0;

  if (PrintBase::Dec == base)
    size = bytestream_detail::format_signed_decimal(value, buffer, sizeof(buffer));
  else
    size = bytestream_detail::format_unsigned((uint32_t)value, base, buffer, sizeof(buffer));

  return bytestream_detail::write_chars(stream, buffer, size);
}

template <ByteStream StreamT>
inline size_t bytestream_print(StreamT& stream, int64_t value, PrintBase base = PrintBase::Dec)
{
  char buffer[bytestream_detail::PRINT_BUFFER_SIZE];
  size_t size = 0;

  if (PrintBase::Dec == base)
    size = bytestream_detail::format_signed_decimal(value, buffer, sizeof(buffer));
  else
    size = bytestream_detail::format_unsigned((uint64_t)value, base, buffer, sizeof(buffer));

  return bytestream_detail::write_chars(stream, buffer, size);
}

template <ByteStream StreamT>
inline size_t bytestream_newline(StreamT& stream)
{
  return bytestream_write_cstr(stream, "\r\n");
}

template <typename DerivedT>
class ByteStreamPrintMixin
{
public:
  size_t print(const char *text)
  {
    return bytestream_print(self(), text);
  }

  size_t print(uint8_t value, PrintBase base = PrintBase::Dec)
  {
    return bytestream_print(self(), value, base);
  }

  size_t print(uint16_t value, PrintBase base = PrintBase::Dec)
  {
    return bytestream_print(self(), value, base);
  }

  size_t print(uint32_t value, PrintBase base = PrintBase::Dec)
  {
    return bytestream_print(self(), value, base);
  }

  size_t print(uint64_t value, PrintBase base = PrintBase::Dec)
  {
    return bytestream_print(self(), value, base);
  }

  size_t print(int8_t value, PrintBase base = PrintBase::Dec)
  {
    return bytestream_print(self(), value, base);
  }

  size_t print(int16_t value, PrintBase base = PrintBase::Dec)
  {
    return bytestream_print(self(), value, base);
  }

  size_t print(int32_t value, PrintBase base = PrintBase::Dec)
  {
    return bytestream_print(self(), value, base);
  }

  size_t print(int64_t value, PrintBase base = PrintBase::Dec)
  {
    return bytestream_print(self(), value, base);
  }

  size_t newline(void)
  {
    return bytestream_newline(self());
  }

private:
  DerivedT& self(void)
  {
    return (DerivedT&)*this;
  }
};

#endif // _BYTESTREAM_HPP_
