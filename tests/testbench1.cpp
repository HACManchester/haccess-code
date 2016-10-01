/* testbench1.cpp
 *
 * Copyright 2016 Ben Dooks <ben@fluff.org>
 */

#include "trigger.h"
#include "timer.h"
#include "trigger_config.h"

#include "FS.h"
#include "IniFile.h"

#include "config.h"

#include "trigger-utils.h"
#include "trigger-script.h"

/* read commands from a file to stimulate bits of the system */

void errExit(const char *msg)
{
  fprintf(stderr, "ERROR: %s\n", msg);
  exit(1);
}

static void show_opto(trigger *trig, bool val)
{
  printf("Door is %s\n", val ? "activated" : "locked");
}

static void set_out(const char *name, void (*fn)(trigger *, bool to))
{
  class trigger *trig;

  trig = trigger_find(name);
  if (!trig) {
    fprintf(stderr, "cannot find trigger '%s'\n", name);
    return;
  }

  trig->notify_fn = fn;
}

#define MAX_ARGS (8)

int main(int argc, char **argv)
{
  File f = File("../data/config.ini");
  char *args[MAX_ARGS+1];
  char *line;
  int rc;
  
  set_config_file(f);
  setup_triggers();

  // setup a few output notifiers
  set_out("output/opto", show_opto);

  // prepare to get going
  start_timer();

  printf("\nStarting command line:\n");
  while (1) {
    line = trigger_readline(">");
    rc = split_cmdline(line, MAX_ARGS, args);

    if (rc < 0)
      break;
    
    rc = parse_command(rc, args);
    if (rc)
      break;
    
    free(line);
  }

  return 0;
}
