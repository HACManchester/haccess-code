// Haccess trigger configuration functions

#include <vector>
#include "IniFile.h"

#include "FS.h"

#include "haccess.h"
#include "trigger.h"
#include "trigger_config.h"
#include "mqtt.h"

#ifdef __TEST
#include <stdio.h>
#define __log(...) printf(__VA_ARGS__)
#else
#define __log(...) do { } while(0)
//#define __log (NULL)
#endif

#if 1
#undef __log
#define __log(x...) Serial.printf(x)
#endif

// triggers

class input_trigger *in_rfid;
class input_trigger *in_rfid_auth;
class input_trigger *in_button1;
class input_trigger *in_button2;
class input_trigger *in_opto;
class output_trigger *out_opto;

static class trigger *get_trig(char *name)
{
    class trigger *trig = trigger_find(name);

    if (!trig)
      Serial.printf("ERROR: failed to find '%s'\n", name);
    return trig;
}

/*
// look for a time-specification, in text format and return time in millis
// for example:
// 10c  => 10 centi-seconds
/  10s  => 10 seconds
// 10m  => 10 minutes
// 10h  => 10 hours
// is it worth having days in here?
*/

static bool parse_time(char *buff, unsigned long *result)
{
  unsigned long tmp;
  char *ep = NULL;

  tmp = strtoul(buff, &ep, 10);
  if (ep == buff)
    return false;

  *result = tmp;
  if (ep[0] < 32)
    return true;

  switch (ep[0]) {
    case 's':
      tmp *= 1000;
      break;

    case 'm':
      if (ep[1] != 's')
        tmp *= 1000 * 60;
      break;

    case 'c':
      tmp *= 10;
      break;

    case 'h':
      tmp *= 1000 * 60 * 60;
      break;

    default:
      __log("ERR: failed to parse time '%s'\n", buff);
      return false;
  }

  *result = tmp;
  return true;
}

static void add_dep(class trigger *src, class trigger *dest)
{
  dest->add_dependency(src);
}

static void add_dep_sr_set(class trigger *src, class trigger *dest)
{
  class sr_trigger *sr = static_cast<class sr_trigger *>(dest);

  sr->add_set(src);
}

static void add_dep_sr_clr(class trigger *src, class trigger *dest)
{
  class sr_trigger *sr = static_cast<class sr_trigger *>(dest);

  sr->add_reset(src);
}


static void read_dependency_srcs(class trigger *target,
				 const char *section,
				 char *pfx,
				 void (*fn)(class trigger *src,
					    class trigger *dest))
{
  class trigger *src;
  char name[20];
  char tmp[96];
  bool read;
  int nr;

  Serial.printf("%s: reading dep-src, target %p\n", section, target);

  if (!target)
    return;

  for (nr = 0; nr < 99; nr++) {
    sprintf(name, nr == 0 ? "%s" : "%s%d", pfx, nr);
    read =cfgfile.getValue(section, name, tmp, sizeof(tmp));
    if (!read) {
      if (nr == 0)
	continue;
      else
	break;
    }

    src = get_trig(tmp);
    __log("DBG: section %s: %s => %p\n", section, tmp, src);
    if (src) {
      fn(src, target);
    } else {
      __log("ERR: no trigger for '%s'\n", tmp);
    }
  }

  __log("%s: dependency sources done\n", section);
}

static bool read_trigger_timer(class timer_trigger *tt, const char *section, char *buff, int buff_sz)
{
  unsigned long time;
  bool ok;
  
  if (!tt)
    return false;

  tt->set_length(1000UL);   // default is 1sec

  ok = cfgfile.getValue(section, "time", buff, buff_sz);
  if (!ok)
    ok = cfgfile.getValue(section, "length", buff, buff_sz);
  if (ok) {
    if (!parse_time(buff, &time)) {
      __log("ERR: failed to parse time %s\n", buff);
      return false;
    }

    __log("time set to %lu\n", time);
    tt->set_length(time);
  }

  return true;
}

static void mqtt_trigger_notify(class trigger *trig, bool state)
{
  class output_trigger *out = static_cast<class output_trigger *>(trig);
  struct mqtt_trigger_data *data;

  data = (struct mqtt_trigger_data *)out->data;
  if (!data) {
    Serial.printf("%s: no data for trigger\n", __func__);
    return;
  }

  if (true) {
    Serial.printf("trigger %p state %d\n", trig, state);
    Serial.printf("trigger topic %s state %d\n", data->topic, state);
  }
    
  mqtt_publish(data->topic, state ? "1" : "0");
}

static void read_trigger(const char *section)
{
  class trigger *trig;
  char tmp[96];
  bool bv = false;

  Serial.printf("reading trigger '%s'\n", section);
  __log("DBG: reading trigger '%s'\n", section);
  
  if (!cfgfile.getValue(section, "type", tmp, sizeof(tmp))) {
    Serial.printf("section %s: failed to get type\n", section);
    goto parse_err;
  }

  __log("DBG: trigger type %s\n", tmp);

  if (strcmp(tmp, "sr") == 0) {
    trig = new sr_trigger();
    if (!trig)
      goto mem_err;

    read_dependency_srcs(trig, section, "set", add_dep_sr_set);
    read_dependency_srcs(trig, section, "clear", add_dep_sr_clr);
  } else if (strcmp(tmp, "or") == 0) {
    trig = new or_trigger();
  } else if (strcmp(tmp, "and") == 0) {
    trig = new and_trigger();
  } else if (strcmp(tmp, "not") == 0) {
    trig = new not_trigger();
  } else if (strcmp(tmp, "input") == 0) {
    // todo - check if we already have it registred
    trig = new input_trigger();
  } else if (strcmp(tmp, "mqtt_in") == 0) {
     trig = new input_trigger();
     // to do add this to a list of triggers acceptable to mqtt
  } else if (strcmp(tmp, "mqtt_out") == 0) {
    struct mqtt_trigger_data *data;
    class output_trigger *out;
    // todo - check if we already have it registed

    trig = out = new output_trigger();
    data = (struct mqtt_trigger_data *)calloc(1, sizeof(struct mqtt_trigger_data));
    if (!trig || !data)
      goto mem_err;

    out->data = (void *)data;
    out->notify_fn = mqtt_trigger_notify;

    if (cfgfile.getValue(section, "topic", tmp, sizeof(tmp))) {
      Serial.printf("%s: topic '%s'\n", section, tmp);

      data->topic = strdup(tmp);
      if (!data->topic)
	      goto mem_err;
      Serial.printf("topic %s created\n", data->topic);
    }
  } else if (strcmp(tmp, "timer") == 0) {
    class timer_trigger *tt = new timer_trigger();

    __log("DBG: new timer created for %s, tt=%p\n", section, tt);
    if (tt) {
      if (!read_trigger_timer(tt, section, tmp, sizeof(tmp))) {
        __log("ERR: trigger %s: cannot parse timer\n", section);
        goto parse_err;
      }
    }
    trig = tt;
  } else {
    Serial.printf("section %s: failed type of '%s'\n", section, tmp);
    trig = NULL;
    goto parse_err;
  }

  __log("DBG: new trigger %p\n", trig);

  if (!trig) {
    Serial.printf("no memory for trigger\n");
    return;
  }

  // do any standard trigger parsing that's common to all triggers

  if (cfgfile.getValue(section, "name", tmp, sizeof(tmp))) {
    char *newname = strdup(tmp);
    if (newname)
      trig->set_name(newname);
    __log("DBG: trigger name set to '%s'\n", newname);
  }

  if (cfgfile.getValue(section, "default", tmp, sizeof(tmp), bv)) {
    Serial.printf("%s: default %d\n", section, bv);
    trig->new_state(bv);
    __log("DBG: trigger name set to '%s'\n", bv ? "on" : "off");
  }

  if (cfgfile.getValue(section, "topic", tmp, sizeof(tmp))) {
    Serial.printf("%s: topic '%s'\n", section, tmp);
    // todo - attach to the mqtt handler?
  }

  if (cfgfile.getValue(section, "repeat", tmp, sizeof(tmp))) {
    unsigned long reptime;

    if (parse_time(tmp, &reptime)) {
      Serial.printf("%s: repeat %ld ms\n", section, reptime);
      // todo - work out how to do this.
      // new timer type?
    }
  }

  if (cfgfile.getValue(section, "edge", tmp, sizeof(tmp), bv)) {
    __log("DBG: edge value %s requested (%s)\n", tmp, bv ? "high" : "low");
    trig->set_edge(true, bv);
  }

  if (cfgfile.getValue(section, "expires", tmp, sizeof(tmp))) {
    unsigned long exptime;

    // create a timer that then goes and un-sets the given
    // trigger.
    if (parse_time(tmp, &exptime)) {
      class timer_trigger *tt = new timer_trigger();
      class forward_trigger *ft = new forward_trigger();

      Serial.printf("%s: expiry %ld ms\n", section, exptime);

      if (!tt || !ft) {
        Serial.printf("%s: failed to create tigger\n");
        goto parse_err;
      }

      tt->set_name("internal/timer");
      ft->set_name("internal/forward");
      tt->set_length(exptime);
      tt->set_edge(true, true);
      tt->add_dependency(trig);  // todo - fix this it crashes
      ft->set_target(trig);
      ft->add_dependency(tt);
    } else {
      __log("DBG: failed to parse expiry\n");
      goto parse_err;
    }
  }

  // note, dependency information can be handled elsewhere
  read_dependency_srcs(trig, section, "source", add_dep);

  return;

mem_err:
  Serial.printf("failed allocating memory for trigger\n");
parse_err:
  Serial.printf("failed parsing '%s'\n", section);
}

class trigger *cfg_lookup_trigger(const char *section, char *name)
{
  //class trigger *trig;
    char tmp[96];

    if (!cfgfile.getValue(section, name, tmp, sizeof(tmp)))
      return NULL;

    return get_trig(tmp);
}

static void read_dependency(const char *section)
{
  class trigger *target = cfg_lookup_trigger(section, "target");
  //class trigger *src;

  if (!target) {
    __log("DBG: no dependency target for '%s'\n", section);
    return;
  }

  read_dependency_srcs(target, section, "source", add_dep);
  // todo - if set/reset, add set/reset dependencies
}

static String get_section(String line)
{
  int end = line.indexOf(']');

  if (end > 1)
    return line.substring(1, end-1+1);
  return "";
}

static void read_triggers(void)
{
  File f = cfg_file;
  String line;
  uint32_t pos = 0;

  Serial.println("reading trigger list");

  if (!f)
    return;

  __log("DBG: %s: f.available=%d\n", __func__, f.available());
  while (f.available()) {
    __log("DBG: read_triggers: position %u\n", pos);
    //Serial.printf("- pos %u\n", pos);

    if (!f.seek(pos, SeekSet)) {
      Serial.println("read_triggers: failed seek\n");
      break;
    }

    line = f.readStringUntil('\n');
    if (!line)
      break;
    // is line null if this fails?
    
    if (line.startsWith("[logic") ||
        line.startsWith("[input") ||
        line.startsWith("[output")) {
      String section = get_section(line);
      read_trigger(section.c_str());
    }
    pos += line.length() + 1;
  }
}

static void read_depends(void)
{
  File f = cfg_file;
  String line;
  uint32_t pos = 0;

  Serial.println("reading dependency info");

  if (!f)
    return;

  while (true) {
    if (!f.seek(pos, SeekSet)) {
      Serial.println("read_triggers: failed seek\n");
      break;
    }

    if (f.available() <= 0)
      break;
    
    line = f.readStringUntil('\n');
    if (!line)
      break;
 
    if (line.startsWith("[dependency")) {
      String section = get_section(line);
      __log("DBG: found depdency section '%s'\n", section.c_str());
      read_dependency(section.c_str());
    }
    pos += line.length() + 1;
  }
}

static void dump_trig_dep(class trigger *trig)
{
  Serial.printf(" Dep %s: %d\n", trig->get_name(), trig->get_state());
}

static void dump_trigger(class trigger *trig)
{
  Serial.printf("Trigger %s: %d\n", trig->get_name(), trig->get_state());
  trig->run_depends(dump_trig_dep);
  Serial.println("");
}

void setup_triggers(void)
{
  Serial.println("setting up triggers");

  in_rfid = new input_trigger();
  in_rfid_auth = new input_trigger();
  in_button1 = new input_trigger();
  in_button2 = new input_trigger();
  in_opto = new input_trigger();

  out_opto = new output_trigger();
  
  in_rfid->set_name("input/rfid");
  in_rfid_auth->set_name("input/rfid/auth");

  in_button1->set_name("input/button1");
  in_button2->set_name("input/button2");
  in_opto->set_name("input/opto");

  out_opto->set_name("output/opto");

  // todo - set output actions.
  if (true) {
    trigger_run_all(dump_trigger);
  }

  /* we run through the configuration files in several passes. As we need
   * unique section names, we need to work through until weve read all
   * the lines */

  Serial.println("reading triggers");
  read_triggers();
  read_depends();

  if (true) {
    trigger_run_all(dump_trigger);
  }
}
