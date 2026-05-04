#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>

#include "serial.hpp"

extern "C" int _write(int file, const void *ptr, size_t len)
{
  if ((file != 1) && (file != 2))
  {
    errno = EBADF;
    return -1;
  }

  if (!Serial.ready())
    return (int)len;

  return (int)Serial.write((const uint8_t *)ptr, len);
}

extern "C" int _read(int file, void *ptr, size_t len)
{
  if (file != 0)
  {
    errno = EBADF;
    return -1;
  }

  if ((0 == len) || !Serial.ready())
    return 0;

  return (int)Serial.read((uint8_t *)ptr, len);
}

extern "C" int _close(int file)
{
  if ((file >= 0) && (file <= 2))
    return 0;

  errno = EBADF;
  return -1;
}

extern "C" int _isatty(int file)
{
  if ((file >= 0) && (file <= 2))
    return 1;

  errno = EBADF;
  return 0;
}

extern "C" int _fstat(int file, struct stat *st)
{
  if ((file < 0) || (file > 2))
  {
    errno = EBADF;
    return -1;
  }

  st->st_mode = S_IFCHR;
  return 0;
}

extern "C" int _lseek(int file, int ptr, int dir)
{
  (void)ptr;
  (void)dir;

  if ((file < 0) || (file > 2))
  {
    errno = EBADF;
    return -1;
  }

  errno = ESPIPE;
  return -1;
}
