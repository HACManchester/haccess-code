
#include "FS.h"

#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_BUFFER_SIZE (4096)

using namespace std;
using namespace fs;

File::File(char *fname)
{
  this->fd = open(fname, O_RDWR);
}


File::operator bool(void)
{
  return true;
}

bool File::seek(uint32_t pos, SeekMode mode)
{
  off_t seeked;
  int why = -1;

  switch (mode) {
  case SeekSet: why = SEEK_SET; break;
  case SeekEnd: why = SEEK_END; break;
  case SeekCur: why = SEEK_CUR; break;
  default:
    fprintf(stderr, "cannot handle seek mode\n");
    return false;
  }
  
  seeked = lseek(this->fd, pos, why);
  return seeked != (off_t)-1;
}

extern "C" ssize_t c_read(int fd, void *, size_t sz);
size_t File::read(uint8_t *buf, size_t sz)
{
  return c_read(this->fd, (void *)buf, sz);
}

int File::available(void)
{
  return 0;
}

extern "C" void c_close(int fd);
void File::close(void)
{
  c_close(this->fd);
}

String File::readStringUntil(char ch)
{
  char buff[MAX_BUFFER_SIZE];
  int ptr;
  ssize_t rd;

  for (ptr = 0; ptr < MAX_BUFFER_SIZE-1; ptr++) {
    rd = read((uint8_t*)buff+ptr, sizeof(char));
    if (rd <= 0)
      break;
    if (buff[ptr] == ch)
      break;
  }

  buff[ptr] = '\0';
  return String(strdup(buff));
}
