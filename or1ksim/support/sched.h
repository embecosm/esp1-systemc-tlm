/* sched.h -- Abstract entities header file handling job scheduler
   Copyright (C) 2001 Marko Mlinar, markom@opencores.org
   Copyright (C) 2005 György `nog' Jeney, nog@sdf.lonestar.org

This file is part of OpenRISC 1000 Architectural Simulator.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#ifndef _SCHED_H_
#define _SCHED_H_

#if DYNAMIC_EXECUTION
#include "sched_i386.h"
#endif

#include "debug.h"

#define SCHED_HEAP_SIZE 128
#define SCHED_TIME_MAX  INT32_MAX

DECLARE_DEBUG_CHANNEL(sched);
DECLARE_DEBUG_CHANNEL(sched_jobs);

/* Structure for holding one job entry */
struct sched_entry {
  int32_t time;          /* Clock cycles before job starts */
  void *param;           /* Parameter to pass to the function */
  void (*func)(void *);  /* Function to call when time reaches 0 */
  struct sched_entry *next;
};

/* Heap of jobs */
struct scheduler_struct {
  struct sched_entry *free_job_queue;
  struct sched_entry *job_queue;
};

void sched_init(void);
void sched_reset(void);
void sched_next_insn(void (*func)(void *), void *dat);

extern struct scheduler_struct scheduler;

static inline void sched_print_jobs(void)
{
  struct sched_entry *cur;
  int i;

  for (cur = scheduler.job_queue, i = 0; cur; cur = cur->next, i++)
    TRACE_(sched)("\t%i: %p $%p @ %"PRIi32"\n", i, cur->func, cur->param,
                  cur->time);
}

/* Adds new job to the queue */
static inline void sched_add(void (*job_func)(void *), void *job_param,
                             int32_t job_time, const char *func)
{
  struct sched_entry *cur, *prev, *new_job;
  int32_t alltime;

  TRACE_(sched)("%s@%lli:SCHED_ADD(time %"PRIi32")\n", func, runtime.sim.cycles,
                job_time);
  if(TRACE_ON(sched_jobs))
    sched_print_jobs();

  if(TRACE_ON(sched_jobs))
    TRACE_(sched) ("--------\n");

  cur = scheduler.job_queue;
  prev = NULL;
  alltime = cur->time;
  while(cur && (alltime < job_time)) {
    prev = cur;
    cur = cur->next;
    if(cur)
      alltime += cur->time;
  }

  new_job = scheduler.free_job_queue;
  scheduler.free_job_queue = new_job->next;
  new_job->next = cur;

  new_job->func = job_func;
  new_job->param = job_param;

  if(prev) {
    new_job->time = job_time - (alltime - (cur ? cur->time : 0));
    prev->next = new_job;
    TRACE_(sched)("Scheduled job not going to head of queue, relative time: %"
                  PRIi32"\n", new_job->time);
  } else {
    scheduler.job_queue = new_job;
    new_job->time = job_time >= 0 ? job_time : cur->time;
#if DYNAMIC_EXECUTION
    /* If we are replaceing the first job in the queue, then update the
     * recompiler's internal cycle counter */
    set_sched_cycle(job_time);
#endif
    TRACE_(sched)("Setting to-go cycles to %"PRIi32" at %lli\n", job_time,
                  runtime.sim.cycles);
  }

  if(cur)
    cur->time -= new_job->time;

  if(TRACE_ON(sched_jobs))
    sched_print_jobs();
}

#define SCHED_ADD(job_func, job_param, job_time) sched_add(job_func, job_param, job_time, __FUNCTION__)

/* Returns a job with specified function and param, NULL if not found */
static inline void sched_find_remove(void (*job_func)(void *), void *dat,
                                     const char *func)
{
  struct sched_entry *cur;
  struct sched_entry *prev = NULL;

  TRACE_(sched)("%s@%lli:SCHED_REMOVE()\n", func, runtime.sim.cycles);

  for (cur = scheduler.job_queue; cur; prev = cur, cur = cur->next) {
    if ((cur->func == job_func) && (cur->param == dat)) {
      if(cur->next)
        cur->next->time += cur->time;

      if(prev) {
        prev->next = cur->next;
      } else {
        scheduler.job_queue = cur->next;
#if DYNAMIC_EXECUTION
        if(cur->next)
          set_sched_cycle(cur->next->time);
#endif
        if(cur->next)
          TRACE_(sched)("Setting to-go cycles to %"PRIi32" at %lli\n",
                        cur->next->time, runtime.sim.cycles);
      }
      cur->next = scheduler.free_job_queue;
      scheduler.free_job_queue = cur;
      break;
    }
  }
}

/* Deletes job with specified function and param */
#define SCHED_FIND_REMOVE(f, p) sched_find_remove(f, p, __FUNCTION__)

/* If we are not running with dynamic execution, then do_scheduler will be
 * inlined and this declaration will be useless (it will also fail to compile)*/
#if DYNAMIC_EXECUTION
void do_scheduler (void);
#endif

#endif /* _SCHED_H_ */
