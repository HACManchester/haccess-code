
#include "timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <signal.h>
#include <time.h>

#include "millis.h"
#include "trigger-utils.h"

void do_sleep(unsigned long sl)
{
  struct timespec ts, rem;
  int ret;

  ts.tv_sec = sl / 1000;
  ts.tv_nsec = (sl % 1000) * 1000;

  do {
    ret = nanosleep(&ts, &rem);
    if (ret == 0)
      break;
    ts = rem;
  } while (ts.tv_sec != 0 && ts.tv_nsec != 0);
}

static void handler(int sig, siginfo_t *si, void *uc)
{
  timer_t tmr = si->si_value.sival_ptr;

  //printf("signal %ld\n", millis());
  timer_sched(millis());
}

/* from the example code for timer_create */
int start_timer(void)
{
  timer_t timerid;
  struct sigevent sev;
  struct itimerspec its;
  long long freq_nanosecs;
  sigset_t mask;
  struct sigaction sa;

  /* Establish handler for timer signal */

  sa.sa_flags = SA_SIGINFO | SA_RESTART;
  sa.sa_sigaction = handler;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIG, &sa, NULL) == -1)
    errExit("sigaction");

  /* Create the timer */

  sev.sigev_notify = SIGEV_SIGNAL;
  sev.sigev_signo = SIG;
  sev.sigev_value.sival_ptr = &timerid;
  if (timer_create(CLOCKID, &sev, &timerid) == -1)
    errExit("timer_create");

  printf("timer ID is 0x%lx\n", (long) timerid);

  /* Start the timer */

  freq_nanosecs = 1000 * 1000 * 100;
  its.it_value.tv_sec = freq_nanosecs / 1000000000;
  its.it_value.tv_nsec = freq_nanosecs % 1000000000;
  its.it_interval.tv_sec = its.it_value.tv_sec;
  its.it_interval.tv_nsec = its.it_value.tv_nsec;

  if (timer_settime(timerid, 0, &its, NULL) == -1)
    errExit("timer_settime");

  return 0;
}
