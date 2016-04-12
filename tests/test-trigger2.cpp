
#include "trigger.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//#include <readline/readline.h>
//#include <readline/history.h>

static char *readline(char *pr)
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
    class output_trigger *otrig;
    otrig = new output_trigger;
    trig = otrig;
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

int main(void)
{
  char *line;
  int ret;
  char *args[3];
  
  while (0==0) {
    
    line = readline(">");
    if (!line)
      break;

    if (line[0] == '#')
      continue;
    
    ret = sscanf(line, "%ms %ms %ms %ms", &args[0], &args[1], &args[2], &args[3]);
    if (ret < 1)
      continue;

    ret--;
    
    if (strcmp(args[0], "add") == 0) {
      process_add(ret, args+1);
    } else if (strcmp(args[0], "set") == 0) {
      process_set(ret, args+1);
    } else if (strcmp(args[0], "get") == 0) {
      process_get(ret, args+1);
    } else if (strcmp(args[0], "dep") == 0) {
      process_dep(ret, args+1);
    } else if (strcmp(args[0], "dep-s") == 0) {
      process_dep_set(ret, args+1);
    } else if (strcmp(args[0], "dep-r") == 0) {
      process_dep_reset(ret, args+1);
    } else if (strcmp(args[0], "dump") == 0) {
      process_dump(ret, args+1);
    } else if (strcmp(args[0], "quit") == 0) {
      puts("goodbye");
      break;
    } else {
      fprintf(stderr, "did not understand '%s'\n", args[0]);
    }
      
    free(line);
  }

  return 0;
}
