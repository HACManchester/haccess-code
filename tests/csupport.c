
#include <stdio.h>
#include <unistd.h>

ssize_t c_read(int fd, void *buff, size_t sz)
{
  return read(fd, buff, sz);
}

