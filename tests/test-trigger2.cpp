
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

//#include <readline/readline.h>
//#include <readline/history.h>

#define MAX_ARGS (8)

void errExit(const char *msg)
{
  fprintf(stderr, "ERROR: %s (%d)\n", msg, errno);
  exit(4);
}

int main(void)
{
  char *line;
  int rc, ret;
  char *args[3];

  start_timer();
  
  while (0==0) {
    line = trigger_readline(">");

    ret = split_cmdline(line, MAX_ARGS, args);
    if (ret < 0)
      break;

    rc = parse_command(ret, args);
    if (rc)
      break;
       
    free(line);
  }

  return 0;
}
