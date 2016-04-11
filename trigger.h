/* trigger.h
 *
 * triggered flow control
 * Copyright 2016 Ben Dooks <ben@fluff.org>
 */

#include <vector>
#include <string>

/* base class for any trigger */
class trigger {
 public:
  trigger();
  ~trigger() { };

  void set_name(char * to) { this->name = to; }
  char *get_name(void) { return this->name; }
  
  bool depends_on(class trigger *trig);
  void depend_change(class trigger *trig);
  
  void new_state(bool state);
  bool get_state(void) { return state; };
  bool get_modify(void) { return modified; };

  void add_dependency(class trigger *trig);

  // default is to do nothing
 protected:
  virtual void notify(bool to) { };
  virtual bool recalc(class trigger *trig) { return false; }

  char * name;
  std::vector<trigger *> depends;
  
  bool state;
  bool modified;
};

#define for_all_triggers(__ptr, __vector) for (__ptr : __vector)

class and_trigger : public trigger {
 protected:
  bool recalc(class trigger *trig);
};

class not_trigger : public trigger {
 protected:
  bool recalc(class trigger *trig);
};

class or_trigger : public trigger {
 protected:
  bool recalc(class trigger *trig);  
};

class sr_trigger : public trigger {
 protected:
  bool recalc(class trigger *trig);

 public:
  void add_set(class trigger *trig) { set.push_back(trig); add_dependency(trig); }
  void add_reset(class trigger *trig) { reset.push_back(trig); add_dependency(trig); }  

 private:
  std::vector<trigger *> reset;
  std::vector<trigger *> set; 
};
