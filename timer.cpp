/* timer.h
 *  timer implementation
 *  Copyright 2016 Ben Dooks
 */

#include <vector>
#include "timer.h"

#define debug true

#ifdef ESP8266
#include <Arduino.h>
#endif

#define for_all_timers(__ptr, __vector) for (it = __vector.begin(), __ptr = (it != __vector.end()) ? *it : NULL; it != __vector.end(); it++, __ptr = *it)

static std::vector<timer *> timers;

void timer::set(unsigned long exptime)
{
  if (debug)
    Serial.printf("setting timer %p to %lu\n", this, exptime);

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

static unsigned long last_time = ~0UL;


void timer_sched(unsigned long curtime)
{
  std::vector<timer *>::iterator it;
  class timer *ptr;

  if (curtime == last_time)
    return;
  last_time = curtime;

  it = timers.begin();
  while (it != timers.end()) {
    ptr = *it;
 
    if (ptr->expired(curtime)) {
      if (debug) {
        Serial.printf("timer %p expired\n", ptr);
      }
      it = timers.erase(it);
      ptr->fire();
    } else {
      it++;
    }
  }
}
