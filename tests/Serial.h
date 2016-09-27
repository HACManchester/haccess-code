
#include <stdarg.h>
#include <stdio.h>

class Serial {
 public:
  static inline size_t println(const char *val) { puts(val); return 0; };

  static inline int printf(const char *msg, ...) {
    va_list va;
    int ret;
    va_start(va, msg);
    ret = vprintf(msg, va);
    va_end(va);
    return ret;
  };   
};

extern class Serial Serial;

