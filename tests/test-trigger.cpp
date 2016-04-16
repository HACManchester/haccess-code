#include "trigger.h"

#include <stdio.h>

unsigned long millis(void)
{
  return 0;  // not using this
}

static void run_test(trigger *a, trigger *b, trigger *result)
{
  a->new_state(false);
  b->new_state(false);
  result->add_dependency(a);
  result->add_dependency(b);

  printf("a=%d, b=%d, result=%d\n",
	 a->get_state() ? 1 : 0,
	 b->get_state() ? 1 : 0,
	 result->get_state() ? 1 : 0);

  a->new_state(true);
  b->new_state(false);

   printf("a=%d, b=%d, result=%d\n",
	 a->get_state() ? 1 : 0,
	 b->get_state() ? 1 : 0,
	 result->get_state() ? 1 : 0);

   a->new_state(false);
   b->new_state(true);

   printf("a=%d, b=%d, result=%d\n",
	  a->get_state() ? 1 : 0,
	  b->get_state() ? 1 : 0,
	  result->get_state() ? 1 : 0);
 
   a->new_state(true);
   b->new_state(true);

   printf("a=%d, b=%d, result=%d\n",
	  a->get_state() ? 1 : 0,
	  b->get_state() ? 1 : 0,
	  result->get_state() ? 1 : 0);

   a->new_state(false);
   b->new_state(false);

   printf("a=%d, b=%d, result=%d\n",
	  a->get_state() ? 1 : 0,
	  b->get_state() ? 1 : 0,
	  result->get_state() ? 1 : 0);

}

int main(void)
{
  class trigger a, b;
  class or_trigger tor;
  class sr_trigger tsr;
  class and_trigger tand;

  a.set_name("a");
  b.set_name("b");
  tor.set_name("or");
  tsr.set_name("sr");
  tand.set_name("and");
  
  printf("a %p, b %p\n", &a, &b);

  printf("\nOR:\n");
  run_test(&a, &b, &tor);
  printf("\nAND:\n");
  run_test(&a, &b, &tand);
  
  /* try an or-trigger for first */

  printf("\nSR:\n");
  a.new_state(false);
  b.new_state(false);
  tsr.add_set(&a);
  tsr.add_reset(&b);
 
  a.new_state(true);
  printf("a=%d, b=%d, result=%d\n",
	 a.get_state(), b.get_state(), tsr.get_state());

  a.new_state(false);
  printf("a=%d, b=%d, result=%d\n",
	 a.get_state(), b.get_state(), tsr.get_state());
  
  b.new_state(true);
  printf("a=%d, b=%d, result=%d\n",
	 a.get_state(), b.get_state(), tsr.get_state());

  b.new_state(false);
  printf("a=%d, b=%d, result=%d\n",
	 a.get_state(), b.get_state(), tsr.get_state());
  
  
  return 0;	 
}
