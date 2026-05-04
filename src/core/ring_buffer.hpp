#ifndef _RING_BUFFER_HPP_
#define _RING_BUFFER_HPP_

#include <stddef.h>
#include <stdint.h>
#include <utility>

template <typename T, uint8_t N>
class RingBuffer
{
  static_assert(N > 0, "RingBuffer exponent N must be > 0");
  static_assert(N < (sizeof(size_t) * 8), "RingBuffer exponent N is too large");

public:
  static constexpr size_t storage_size = (size_t(1) << N);
  static constexpr size_t capacity_value = storage_size - 1;

  RingBuffer(void) : head_(0), tail_(0)
  {
  }

  static constexpr size_t capacity(void)
  {
    return capacity_value;
  }

  size_t size(void) const
  {
    return (head_ - tail_) & mask();
  }

  bool empty(void) const
  {
    return head_ == tail_;
  }

  bool full(void) const
  {
    return next(head_) == tail_;
  }

  void clear(void)
  {
    head_ = 0;
    tail_ = 0;
  }

  // Returns true if the oldest element was overwritten.
  bool push(const T& value)
  {
    bool overwritten = full();

    data_[head_] = value;
    head_ = next(head_);

    if (overwritten)
      tail_ = next(tail_);

    return overwritten;
  }

  // Returns true if the oldest element was overwritten.
  bool push(T&& value)
  {
    bool overwritten = full();

    data_[head_] = std::move(value);
    head_ = next(head_);

    if (overwritten)
      tail_ = next(tail_);

    return overwritten;
  }

  template <typename... Args>
  bool emplace(Args&&... args)
  {
    bool overwritten = full();

    data_[head_] = T(std::forward<Args>(args)...);
    head_ = next(head_);

    if (overwritten)
      tail_ = next(tail_);

    return overwritten;
  }

  bool pop(T& out)
  {
    if (empty())
      return false;

    out = std::move(data_[tail_]);
    tail_ = next(tail_);
    return true;
  }

  bool peek(T& out) const
  {
    if (empty())
      return false;

    out = data_[tail_];
    return true;
  }

  T* front(void)
  {
    return empty() ? 0 : &data_[tail_];
  }

  const T* front(void) const
  {
    return empty() ? 0 : &data_[tail_];
  }

private:
  static constexpr size_t mask(void)
  {
    return storage_size - 1;
  }

  static size_t next(size_t index)
  {
    return (index + 1) & mask();
  }

  T data_[storage_size];
  size_t head_;
  size_t tail_;
};

#endif // _RING_BUFFER_HPP_
