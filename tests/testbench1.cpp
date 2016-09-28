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

/* read commands from a file to stimulate bits of the system */

void errExit(const char *msg)
{
  fprintf(stderr, "ERROR: %s\n", msg);
  exit(1);
}

int main(int argc, char **argv)
{
  File f = File("../data/config.ini");

  set_config_file(f);
  setup_triggers();

  // prepare to get going
  start_timer();
  
  while (1) {
    
  }

  return 0;
}
