/* timer.h
 *  timer implementation
 *  Copyright 2016 Ben Dooks
 */

#include <vector>
#include "timer.h"

#ifdef ESP8266
#include <Arduino.h>
#endif

#define for_all_timers(__ptr, __vector) for (it = __vector.begin(), __ptr = (it != __vector.end()) ? *it : NULL; it != __vector.end(); it++, __ptr = *it)

static std::vector<timer *> timers;

void timer::set(unsigned long exptime)
{
  if (!this->running) {
    this->running = true;
    timers.push_back(this);
  }

  this->start = millis();
  this->expires = exptime;
}

void timer::fire(void)
{
  this->running = false;
  this->fn(this->param);
}

void timer_sched(unsigned long curtime)
{
  std::vector<timer *>::iterator it;
  class timer *ptr;

  it = timers.begin();
  while (it != timers.end()) {
    ptr = *it;
 
    if (ptr->expired(curtime)) {
      it = timers.erase(it);
      ptr->fire();
    } else {
      it++;
    }
  }
}

