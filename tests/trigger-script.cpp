
#include "trigger.h"
#include "timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <signal.h>
#include <time.h>

#include "millis.h"
#include "trigger-utils.h"
#include "trigger-script.h"

char *trigger_readline(char *pr)
{
  char buff[1024];
  char *ptr, *st;
  
  printf("%s", pr);

  ptr = fgets(buff, sizeof(buff)-1, stdin);
  if (!ptr)
    return NULL;

  st = strdup(ptr);
  return st;
}

static trigger *get_trig(char *name)
{
  class trigger *trig;

  trig = trigger_find(name);
  if (!trig)
    fprintf(stderr, "could not find trigger '%s'\n", name);
  else
    printf("{ trig %s => %p }\n", name, trig);
  
  return trig;
}

static void output_fn(class trigger *trig, bool to)
{
  printf("<< trigger %s is now %d\n", trig->get_name(), to);
}

static int process_add(int argc, char **args)
{
  class trigger *trig;
  char *name;
  
  if (argc < 2)
    return 1;

  if (strcmp(args[0], "or") == 0) {
    trig = new or_trigger;
  } else if (strcmp(args[0], "sr") == 0) {
    trig = new sr_trigger;
  } else if (strcmp(args[0], "and") == 0) {
    trig = new and_trigger;
  } else if (strcmp(args[0], "not") == 0) {
    trig = new not_trigger;
  } else if (strcmp(args[0], "input") == 0) {
    trig = new input_trigger;
  } else if (strcmp(args[0], "output") == 0) {
    trig = new output_trigger;
    trig->notify_fn = output_fn;
  } else if (strcmp(args[0], "timer") == 0) {
    class timer_trigger *tt = new timer_trigger;

    tt->set_length(1000UL);   // default is 1sec
    trig = tt;
  } else {
    fprintf(stderr, "unknown triger type '%s'\n", args[0]);
    return 2;
  }

  if (!trig) {
    fprintf(stderr, "failed to create trigger\n");
    return 2;
  }

  name = strdup(args[1]);
  if (!name) {
    fprintf(stderr, "failed to make name\n");
    return 3;
  }

  printf("=> adding trigger '%s'\n", name);
  trig->set_name(name);
  
  return 0;
}

static int process_dep(int argc, char **args)
{
  class trigger *trig, *dep;

  printf("{ adding dep %s to %s }\n", args[1], args[0]); 
  if (argc < 2) {
    fprintf(stderr, "args only %d\n", argc);
    return 1;
  }
  
  trig = get_trig(args[0]);
  dep = get_trig(args[1]);

  if (!trig || !dep) {
    fprintf(stderr, "lookup failed\n");
    return 2;
  }

  trig->add_dependency(dep);
  return 0;
}

static int process_dep_set(int argc, char **args)
{
  class trigger *trig, *dep;
  class sr_trigger *sr;
  
  if (argc < 2)
    return 1;
  
  trig = get_trig(args[0]);
  dep = get_trig(args[1]);

  if (!trig || !dep)
    return 2;

  sr = (sr_trigger *)trig;
  sr->add_set(dep);
  return 0;
}

static int process_dep_reset(int argc, char **args)
{
  class trigger *trig, *dep;
  class sr_trigger *sr;
  
  if (argc < 2)
    return 1;
  
  trig = get_trig(args[0]);
  dep = get_trig(args[1]);

  if (!trig || !dep)
    return 2;

  sr = (sr_trigger *)trig;
  sr->add_reset(dep);
  return 0;
}

static void dump_dep(class trigger *trig)
{
  printf("- dep: trigger '%s' value %d\n", trig->get_name(), trig->get_state());
}

static void dump_fn(class trigger *trig)
{
  printf("trigger '%s' value %d\n", trig->get_name(), trig->get_state());

  trig->run_depends(dump_dep);
  printf("\n");
}

static int process_dump(int argc, char **args)
{
  trigger_run_all(dump_fn);
  return 0;
}

static int process_get(int argc, char **args)
{
  return -1;
}

static int process_set(int argc, char **args)
{
  class trigger *trig;
  bool state = false;
  
  if (argc < 2)
    return 1;

  trig = trigger_find(args[0]);
  if (!trig) {
    fprintf(stderr, "could not find trigger '%s'\n", args[0]);
    return 2;
  }

  if (strcmp(args[1], "1") == 0 || strcmp(args[1], "true") == 0 ||
      strcmp(args[1], "on") == 0)
    state = true;
  
  trig->new_state(state);  
  return 0;
}


int split_cmdline(char *line, int max, char **args)
{
  int ret;
    
  if (!line)
    return -1;

  if (line[0] == '#')
    return 0;
    
  ret = sscanf(line, "%ms %ms %ms %ms",
	       &args[0], &args[1], &args[2], &args[3]);
  
  ret--;
  if (ret < 0)
    return 0;
  
  return ret;
}
  
int parse_command(int nr_args, char **args)
{
  if (strcmp(args[0], "add") == 0) {
    process_add(nr_args, args+1);
  } else if (strcmp(args[0], "set") == 0) {
    process_set(nr_args, args+1);
  } else if (strcmp(args[0], "get") == 0) {
    process_get(nr_args, args+1);
  } else if (strcmp(args[0], "dep") == 0) {
    process_dep(nr_args, args+1);
  } else if (strcmp(args[0], "dep-s") == 0) {
    process_dep_set(nr_args, args+1);
  } else if (strcmp(args[0], "dep-r") == 0) {
    process_dep_reset(nr_args, args+1);
  } else if (strcmp(args[0], "sleep") == 0) {
    unsigned long l = strtoul(args[1], NULL, 10);
    do_sleep(l);
  } else if (strcmp(args[0], "dump") == 0) {
    process_dump(nr_args, args+1);
  } else if (strcmp(args[0], "quit") == 0) {
    puts("goodbye");
    return 1;
  } else {
    fprintf(stderr, "did not understand '%s'\n", args[0]);
  }

  return 0;
}
