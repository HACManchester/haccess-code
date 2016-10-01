/* trigger.cpp
 *
 *
 * Copyright 2016 Ben Dooks <ben@fluff.org>
 */

#include <vector>
#include <string.h>

#ifdef __TEST
#include <stdio.h>
extern "C" void __debug_printf(const char *msg, ...);
#define __log(...) printf(__VA_ARGS__)
#else
#define __log(...) do { } while(0)
//#define __log (NULL)
#endif

#include "trigger.h"

#define for_all_triggers(__ptr, __vector) for (it = __vector.begin(), __ptr = (it != __vector.end()) ? *it : NULL; it != __vector.end(); it++, __ptr = *it)

static std::vector<trigger *> triggers;

class trigger *trigger_find(const char *name)
{
  std::vector<class trigger *>::iterator it;
  class trigger *ptr;

  for_all_triggers(ptr, triggers) {
    if (strcmp(ptr->get_name(), name) == 0)
      return ptr;
  }

  return NULL;
}

extern void trigger_run_all(void (*fn)(class trigger *trig))
{
  std::vector<class trigger *>::iterator it;
  class trigger *ptr;

  for_all_triggers(ptr, triggers)
    fn(ptr);
}

void trigger::run_depends(void (*fn)(class trigger *trig))
{
  std::vector<class trigger *>::iterator it;
  class trigger *ptr;

  for_all_triggers(ptr, this->depends)
    fn(ptr);  
}

trigger::trigger()
{
  __log("DBG: new trigger %p\n", this);
  name = "anon";
  state = false;
  modified = false;
  triggers.push_back(this);
}

void trigger::new_state(bool to)
{
  std::vector<class trigger *>::iterator it;
  bool prev_state = this->state;
  class trigger *ptr;

  __log("DBG: %s, %s new_state %d (was %d)\n",
	 __func__, this->name, to, this->state);
  if (to == this->state)
    return;

  this->state = to;
  this->modified = true;

  this->notify(to);

  for_all_triggers(ptr, triggers) {
    if (ptr->depends_on(this))
      ptr->depend_change(this, prev_state);
  }

  this->modified = false;
}

void trigger::add_dependency(class trigger *trig)
{
  __log("DBG: trigger %s depends on %s\n", this->name, trig->name);
  this->depends.push_back(trig);
  this->depend_change(trig, trig->get_state());  // force re-calculation of state //
}

void trigger::depend_change(class trigger *trig, bool prev)
{
  bool nstate = this->recalc(trig, prev);
  __log("DBG: %s: this %s. trig %s: %d\n", __func__, this->name, trig->name, nstate);
  this->new_state(nstate);
}

void trigger::notify(bool to)
{
  if (notify_fn)
    (notify_fn)(this, to);
}

static bool find_in_triggers(class trigger *trig, std::vector<trigger *> list)
{
  std::vector<class trigger *>::iterator it;
  class trigger *ptr;

  for_all_triggers(ptr, list) {
    if (trig == ptr)
      return true;
  }

  return false;
}

bool trigger::depends_on(class trigger *trig)
{
  return find_in_triggers(trig, depends);
}

/* for the moment we'll stick the trigger code in here */

bool and_trigger::recalc(class trigger *trig, bool prev)
{
  std::vector<class trigger *>::iterator it;
  class trigger *ptr;

  // todo - should none return true or false?
  for_all_triggers(ptr, this->depends) {
    if (!ptr->get_state())
      return false;
  }

  return true;
}

bool not_trigger::recalc(class trigger *trig, bool prev)
{
  if (depends.size() < 1)
    return false;

  return !depends[0]->get_state();
}

bool or_trigger::recalc(class trigger *trig, bool prev)
{
  std::vector<class trigger *>::iterator it;
  class trigger *ptr;

  __log("or_trigger: dep change %s by %s\n", this->get_name(), trig->get_name());
  
  // todo - should none return true or false?
  for_all_triggers(ptr, this->depends) {
    __log("or_trigger: dep %s is %d\n", ptr->get_name(), ptr->get_state());
    
    if (ptr->get_state())
      return true;
  }

  return false;
}

bool sr_trigger::recalc(class trigger *trig, bool prev)
{
  std::vector<class trigger *>::iterator it;
  class trigger *ptr;

  for_all_triggers(ptr, reset) {
    if (ptr->get_state())
      return false;
  }

  for_all_triggers(ptr, set) {
    if (ptr->get_state())
      return true;
  }

  return state;
};

bool output_trigger::recalc(class trigger *trig, bool prev)
{
  std::vector<class trigger *>::iterator it;
  class trigger *ptr;

  __log("output_trigger: dep change %s by %s\n", this->get_name(), trig->get_name());

  if (this->depends.size() == 0)
    return false;

  for_all_triggers(ptr, this->depends) {
    __log("output_trigger: dep %s is %d\n", ptr->get_name(), ptr->get_state());

    if (ptr->get_state())
      return true;
  }

  return false;
}

bool trigger::should_trigger(class trigger *trig, bool prev)
{
  if (!this->trig_edge) {
    return (!trig->get_state() && this->get_state());
  }

  if (prev && !this->get_state())
    return !this->trig_val;

  if (!prev && this->get_state());
    return this->trig_val;

  return false;
}

bool timer_trigger::recalc(class trigger *trig, bool prev)
{
  // return true if our dependency is set and we're level triggered
  if (trig->get_state() && !this->trig_edge)
    return true;

  if (should_trigger(trig, prev)) {
    __log("timer_trigger: dep - fire for %lu\n", this->len);
    this->timer.set(this->len);
    return true;
  }

  return this->state;
}

static void trig_expiry_fn(void *ptr)
{
  class timer_trigger *tr = (class timer_trigger *)ptr;
  tr->new_state(false);
}

timer_trigger::timer_trigger(void) : trigger()
{
  timer.expiry(trig_expiry_fn, this);
}

forward_trigger::forward_trigger()
{
  this->f_off = false;
  this->f_on = false;
}

bool forward_trigger::recalc(class trigger *trig, bool prev)
{
  bool state = trig->get_state();

  if ((!state && this->f_off) ||
      (state && this->f_on)) {
     target->new_state(state);
  }
}

