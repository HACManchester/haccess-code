/* timer.h
 *  timer implementation
 *  Copyright 2016 Ben Dooks
 */

#include <vector>


static inline bool has_expired(unsigned long curtime, unsigned long lasttime, unsigned long interval)
{
  return ((curtime - lasttime) >= interval);
}

class timer {
  public:
    void fire(void);
    void set(unsigned long ms);
    void expiry(void (*fn)(void *param), void *param) {
      this->fn = fn;
      this->param = param;
    };
    bool expired(unsigned long cur) {
      return has_expired(cur, this->start, this->expires);
    };
  protected:
    void (*fn)(void *param);
    void *param;
  private:
    unsigned long expires;
    unsigned long start;
    bool running;
};

extern void timer_sched(unsigned long ms);

#ifndef ESP8266
extern unsigned long millis(void);
#endif

