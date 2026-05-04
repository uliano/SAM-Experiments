#ifndef _LINE_READER_HPP_
#define _LINE_READER_HPP_

#include <stddef.h>
#include <stdint.h>

#include "timebase.hpp"

template <typename StreamT>
class LineReader
{
public:
  enum class Status : uint8_t
  {
    Pending = 0,
    Ready,
    Overflow,
    Timeout,
    InvalidArgs,
  };

  struct PollResult
  {
    Status status;
    size_t length;
  };

  LineReader(StreamT& stream, char *buffer, size_t capacity, bool echo = true) :
    stream_(stream),
    buffer_(buffer),
    capacity_(capacity),
    length_(0),
    echo_(echo),
    started_(false),
    skip_lf_once_(false),
    line_ready_(false),
    overflow_(false),
    timeout_ms_(0),
    start_ms_(0)
  {
    if (has_valid_storage())
      buffer_[0] = '\0';
  }

  void reset(void)
  {
    length_ = 0;
    started_ = false;
    line_ready_ = false;
    overflow_ = false;
    start_ms_ = 0;

    if (has_valid_storage())
      buffer_[0] = '\0';
  }

  void consume_line(void)
  {
    reset();
  }

  void set_echo(bool enabled)
  {
    echo_ = enabled;
  }

  void set_timeout_ms(uint32_t timeout_ms)
  {
    timeout_ms_ = timeout_ms;
  }

  PollResult poll(void)
  {
    if (!has_valid_storage())
      return { Status::InvalidArgs, 0 };

    if (!stream_.ready())
      return { Status::Pending, 0 };

    if (line_ready_)
      return current_status();

    if (started_ && timeout_enabled() && timeout_expired(Timebase::millis()))
    {
      const size_t current_length = length_;
      reset();
      return { Status::Timeout, current_length };
    }

    while (stream_.available() > 0)
    {
      uint8_t ub = 0;
      if (1 != stream_.read(&ub, 1))
        break;

      char c = (char)ub;

      if (!started_)
        mark_started(Timebase::millis());

      if (skip_lf_once_ && ('\n' == c))
      {
        skip_lf_once_ = false;
        continue;
      }
      skip_lf_once_ = false;

      if (('\r' == c) || ('\n' == c))
      {
        if ('\r' == c)
          skip_lf_once_ = true;

        if (echo_)
          stream_.newline();

        finish_line();
        return current_status();
      }

      if (('\b' == c) || (0x7f == (uint8_t)c))
      {
        handle_backspace();
        continue;
      }

      if (!is_accepted_char(c))
        continue;

      if (length_ < (capacity_ - 1))
      {
        buffer_[length_++] = c;
        buffer_[length_] = '\0';

        if (echo_)
          stream_.write((const uint8_t *)&c, 1);
      }
      else
      {
        overflow_ = true;
      }
    }

    if (started_ && timeout_enabled() && timeout_expired(Timebase::millis()))
    {
      const size_t current_length = length_;
      reset();
      return { Status::Timeout, current_length };
    }

    return { Status::Pending, length_ };
  }

  bool line_ready(void) const
  {
    return line_ready_;
  }

  const char *c_str(void) const
  {
    if (!has_valid_storage())
      return "";

    return buffer_;
  }

  size_t length(void) const
  {
    return length_;
  }

private:
  bool has_valid_storage(void) const
  {
    return (0 != buffer_) && (capacity_ >= 2);
  }

  bool timeout_enabled(void) const
  {
    return timeout_ms_ > 0;
  }

  bool timeout_expired(uint32_t now_ms) const
  {
    return (uint32_t)(now_ms - start_ms_) >= timeout_ms_;
  }

  void mark_started(uint32_t now_ms)
  {
    started_ = true;
    start_ms_ = now_ms;
  }

  void handle_backspace(void)
  {
    if (0 == length_)
      return;

    --length_;
    buffer_[length_] = '\0';

    if (echo_)
    {
      static const uint8_t backspace_seq[] = { '\b', ' ', '\b' };
      stream_.write(backspace_seq, sizeof(backspace_seq));
    }
  }

  void finish_line(void)
  {
    line_ready_ = true;
    started_ = false;
  }

  PollResult current_status(void) const
  {
    if (!line_ready_)
      return { Status::Pending, length_ };

    if (overflow_)
      return { Status::Overflow, length_ };

    return { Status::Ready, length_ };
  }

  static bool is_accepted_char(char c)
  {
    return ((c >= 0x20) && (c <= 0x7e)) || ('\t' == c);
  }

  StreamT& stream_;
  char *buffer_;
  size_t capacity_;
  size_t length_;
  bool echo_;
  bool started_;
  bool skip_lf_once_;
  bool line_ready_;
  bool overflow_;
  uint32_t timeout_ms_;
  uint32_t start_ms_;
};

#endif // _LINE_READER_HPP_
