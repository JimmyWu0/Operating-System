/*
 * file: preemptive.h
 *
 * this is the include file for the preemptive multithreading
 * package.  It is to be compiled by SDCC and targets the EdSim51 as
 * the target architecture.
 *
 * CS 3423 Fall 2018
 */

#ifndef __PREEMPTIVE_H__
#define __PREEMPTIVE_H__

#define MAXTHREADS 4  /* not including the scheduler */
/* the scheduler does not take up a thread of its own */

#define CNAME(s) _ ## s
#define LABELNAME(label) label ## $

void SemaphoreCreate(char *s, char n) {
   //__critical {
      *s = n;
   //}
   return;
}

#define SemaphoreSignal(s) { \
   __asm \
      INC CNAME(s) \
   __endasm; \
}

#define SemaphoreWaitBody(s, label) { \
   __asm \
      LABELNAME(label): MOV ACC, CNAME(s) \
                        JZ LABELNAME(label) \
                        DEC CNAME(s) \
   __endasm; \
}

#define SemaphoreWait(s) { \
   SemaphoreWaitBody(s, __COUNTER__) \
}

typedef char ThreadID;
typedef void (*FunctionPtr)(void);

ThreadID ThreadCreate(FunctionPtr);
void ThreadYield(void);
void ThreadExit(void);


#endif // __PREEMPTIVE_H__
