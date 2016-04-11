/* trigger.cpp
 *
 *
 * Copyright 2016 Ben Dooks <ben@fluff.org>
 */

#ifdef __TEST
#include <stdio.h>
#define __log(x..) printf(x)
#else
#define __log(x..) do { } while(0)
#endif

#include "trigger.h"

static std::vector<trigger *> triggers;

trigger::trigger()
{
  name = "anon";
  state = false;
  modified = false;
  triggers.push_back(this);
}

void trigger::new_state(bool to)
{
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
      ptr->depend_change(this);
  }

  this->modified = false;
}

void trigger::add_dependency(class trigger *trig)
{
  this->depends.push_back(trig);
  this->depend_change(trig);  // force re-calculation of state //
}

void trigger::depend_change(class trigger *trig)
{
  bool nstate = this->recalc(trig);
  __log("DBG: %s: this %s. trig %s: %d\n", __func__, this->name, trig->name, nstate);
  this->new_state(nstate);
}

static bool find_in_triggers(class trigger *trig, std::vector<trigger *> list)
{
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

bool and_trigger::recalc(class trigger *trig)
{
  class trigger *ptr;

  // todo - should none return true or false?
  for_all_triggers(ptr, this->depends) {
    if (!ptr->get_state())
      return false;
  }

  return true;
}

bool not_trigger::recalc(class trigger *trig)
{
  if (depends.size() < 1)
    return false;

  return !depends[0]->get_state();
}

bool or_trigger::recalc(class trigger *trig)
{
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

bool sr_trigger::recalc(class trigger *trig)
{
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
