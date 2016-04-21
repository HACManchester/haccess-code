/* trigger.h
 *
 * triggered flow control
 * Copyright 2016 Ben Dooks <ben@fluff.org>
 */

#include <iterator>
#include <vector>
#include <string.h>

#include "timer.h"

/* base class for any trigger */
class trigger {
 public:
  trigger();
  ~trigger() { };

  void set_name(char * to) { this->name = to; }
  char *get_name(void) { return this->name; }
  
  bool depends_on(class trigger *trig);
  void depend_change(class trigger *trig, bool prev);
 
  void new_state(bool state);
  bool get_state(void) { return state; };
  bool get_modify(void) { return modified; };

  void add_dependency(class trigger *trig);
  void run_depends(void (*fn)(class trigger *trig));

  void (*notify_fn)(class trigger *trig, bool to);

  // edge/vs level triggers (may not work for all triggers //
  void set_edge(bool to, bool val) { this->trig_edge = to; this->trig_val = val; };

  // if we're an input, then this is true, otherwise false
  virtual bool is_input(void) { return false; };
  virtual bool is_output(void) { return false; };

  // default is to do nothing
 protected:
  bool should_trigger(class trigger *trig, bool prev);

  virtual void notify(bool to);
  virtual bool recalc(class trigger *trig, bool prev) { return false; }

  char * name;
  std::vector<trigger *> depends;
  
  bool state;
  bool modified;
  bool trig_val;      // trigger val (if edge) //
  bool trig_edge;     // set if trigger on edge //
};

extern class trigger *trigger_find(const char *name);
extern void trigger_run_all(void (*fn)(class trigger *trig));

class input_trigger : public trigger {
public:
  virtual bool is_input(void) { return true; };
};

class output_trigger : public trigger {
public:
  virtual bool is_output(void) { return true; };
protected:
  bool recalc(class trigger *trig, bool prev);
};

class and_trigger : public trigger {
 protected:
  bool recalc(class trigger *trig, bool prev);
};

class not_trigger : public trigger {
 protected:
  bool recalc(class trigger *trig, bool prev);
};

class or_trigger : public trigger {
 protected:
  bool recalc(class trigger *trig, bool prev);  
};

class sr_trigger : public trigger {
 protected:
  bool recalc(class trigger *trig, bool prev);

 public:
  void add_set(class trigger *trig) { set.push_back(trig); add_dependency(trig); };
  void add_reset(class trigger *trig) { reset.push_back(trig); add_dependency(trig); };

 private:
  std::vector<trigger *> reset;
  std::vector<trigger *> set; 
};

class timer_trigger : public trigger {
 public:
  timer_trigger();

  void set_length(unsigned long len) { this->len = len; };
  unsigned get_length(void) { return this->len; };

 protected:
  bool recalc(class trigger *trig, bool prev);

 private:
  unsigned long len;
  class timer timer;
};

// allow forwarding change actions without having a dependency
class forward_trigger: public trigger {
public:
  forward_trigger();

  void set_target(class trigger *trig) { this->target = trig; };
  void set_filter(bool off, bool on) { this->f_off = off; this->f_on = on; };
 protected:
  bool f_on, f_off;
  class trigger *target;

  bool recalc(class trigger *trig, bool prev);
};

