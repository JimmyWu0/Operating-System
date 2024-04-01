#include <8051.h>

#include "preemptive.h"

/*
 * @@@ [2 pts] declare the static globals here using 
 *        __data __at (address) type name; syntax
 * manually allocate the addresses of these variables, for
 * - saved stack pointers (MAXTHREADS)
 * - current thread ID
 * - a bitmap for which thread ID is a valid thread; 
 *   maybe also a count, but strictly speaking not necessary
 * - plus any temporaries that you need.
 * 
 */
__data __at (0x30) char stack_pointers_for_threads[MAXTHREADS];
__data __at (0x34) char bitmap_for_threads;
__data __at (0x35) ThreadID current_thread_ID;
__data __at (0x36) char tmp;
__data __at (0x37) char i; // variable for loops
__data __at (0x38) char created_thread_ID; 

/*
 * @@@ [8 pts]
 * define a macro for saving the context of the current thread by
 * 1) push ACC, B register, Data pointer registers (DPL, DPH), PSW
 * 2) save SP into the saved Stack Pointers array
 *   as indexed by the current thread ID.
 * Note that 1) should be written in assembly, 
 *     while 2) can be written in either assembly or C
 */
#define SAVESTATE \
        {\
            __asm\
            PUSH ACC\
            PUSH B\
            PUSH DPL\
            PUSH DPH\
            PUSH PSW\
            __endasm;\
            stack_pointers_for_threads[current_thread_ID] = SP;\
        }

/*
 * @@@ [8 pts]
 * define a macro for restoring the context of the current thread by
 * essentially doing the reverse of SAVESTATE:
 * 1) assign SP to the saved SP from the saved stack pointer array
 * 2) pop the registers PSW, data pointer registers, B reg, and ACC
 * Again, popping must be done in assembly but restoring SP can be
 * done in either C or assembly.
 */
#define RESTORESTATE\
         {\
            SP = stack_pointers_for_threads[current_thread_ID];\
            __asm\
            POP PSW\
            POP DPH\
            POP DPL\
            POP B\
            POP ACC\
            __endasm;\
         }


 /* 
  * we declare main() as an extern so we can reference its symbol
  * when creating a thread for it.
  */

extern void main(void);

/*
 * Bootstrap is jumped to by the startup code to make the thread for
 * main, and restore its context so the thread can run.
 */

void Bootstrap(void) {
      /*
       * @@@ [2 pts] 
       * initialize data structures for threads (e.g., mask)
       *
       * optional: move the stack pointer to some known location
       * only during bootstrapping. by default, SP is 0x07.
       *
       * @@@ [2 pts]
       *     create a thread for main; be sure current thread is
       *     set to this thread ID, and restore its context,
       *     so that it starts running main().
       */
    TMOD = 0;  // timer 0 mode 0
    IE = 0x82;  // enable timer 0 interrupt; keep consumer polling
                // EA  -  ET2  ES  ET1  EX1  ET0  EX0
    TR0 = 1; // set bit TR0 to start running timer 0


    bitmap_for_threads = 0;
    for (i = 0; i < MAXTHREADS; i++)  
        stack_pointers_for_threads[i] = 0;
    current_thread_ID = ThreadCreate(main); // 00B3
      


    RESTORESTATE
}

/*
 * ThreadCreate() creates a thread data structure so it is ready
 * to be restored (context switched in).
 * The function pointer itself should take no argument and should
 * return no argument.
 */
ThreadID ThreadCreate(FunctionPtr fp) {
        /*
         * @@@ [2 pts] 
         * check to see we have not reached the max #threads.
         * if so, return -1, which is not a valid thread ID.
         */
        /*
         * @@@ [5 pts]
         *     otherwise, find a thread ID that is not in use,
         *     and grab it. (can check the bit mask for threads),
         *
         * @@@ [18 pts] below
         * a. update the bit mask 
             (and increment thread count, if you use a thread count, 
              but it is optional)
           b. calculate the starting stack location for new thread
           c. save the current SP in a temporary
              set SP to the starting location for the new thread
           d. push the return address fp (2-byte parameter to
              ThreadCreate) onto stack so it can be the return
              address to resume the thread. Note that in SDCC
              convention, 2-byte ptr is passed in DPTR.  but
              push instruction can only push it as two separate
              registers, DPL and DPH.
           e. we want to initialize the registers to 0, so we
              assign a register to 0 and push it four times
              for ACC, B, DPL, DPH.  Note: push #0 will not work
              because push takes only direct address as its operand,
              but it does not take an immediate (literal) operand.
           f. finally, we need to push PSW (processor status word)
              register, which consist of bits
               CY AC F0 RS1 RS0 OV UD P
              all bits can be initialized to zero, except <RS1:RS0>
              which selects the register bank.  
              Thread 0 uses bank 0, Thread 1 uses bank 1, etc.
              Setting the bits to 00B, 01B, 10B, 11B will select 
              the register bank so no need to push/pop registers
              R0-R7.  So, set PSW to 
              00000000B for thread 0, 00001000B for thread 1,
              00010000B for thread 2, 00011000B for thread 3.
           g. write the current stack pointer to the saved stack
              pointer array for this newly created thread ID
           h. set SP to the saved SP in step c.
           i. finally, return the newly created thread ID.
         */
        if (bitmap_for_threads == 15) return -1;

        __critical{
        EA = 0;

         tmp = MAXTHREADS; // unused thread
         for (i = 0; i < MAXTHREADS; i++) {
            if ((bitmap_for_threads & (1 << i)) == 0) {
               tmp = i;
               break;
            }
         }
         
         created_thread_ID = tmp;

         // a.
         bitmap_for_threads |= 1 << created_thread_ID;

         // b, c
         tmp = SP; // save current SP
         SP = 0x40 + created_thread_ID * 16 - 1;

         // d. intel: little endian, push lower address first(DPL)         
         __asm
            PUSH DPL
            PUSH DPH
         __endasm;

         // e.
         __asm
            MOV A, #0
            PUSH ACC
            PUSH ACC
            PUSH ACC
            PUSH ACC
         __endasm;

         // f.
         PSW = created_thread_ID << 3;
         __asm
            PUSH PSW
         __endasm;

         // g.
         stack_pointers_for_threads[created_thread_ID] = SP;

         // h.
         SP = tmp;
        
        EA = 1;
        }
         // i.
         return created_thread_ID;
}



/*
 * this is called by a running thread to yield control to another
 * thread.  ThreadYield() saves the context of the current
 * running thread, picks another thread (and set the current thread
 * ID to it), if any, and then restores its state.
 */

void ThreadYield(void) {
    __critical{
    EA = 0;
       SAVESTATE;


       do {
                /*
                 * @@@ [8 pts] do round-robin policy for now.
                 * find the next thread that can run and 
                 * set the current thread ID to it,
                 * so that it can be restored (by the last line of 
                 * this function).
                 * there should be at least one thread, so this loop
                 * will always terminate.
                 */


                /*if (current_thread_ID == (MAXTHREADS - 1)) current_thread_ID = 0;
                else current_thread_ID++;

                if (bitmap_for_threads & (1 << current_thread_ID)) break;*/

                // 換個寫法就可以了???
               current_thread_ID++;
               if (current_thread_ID >= MAXTHREADS) current_thread_ID = 0;

                if (current_thread_ID == 0) {
                  if (bitmap_for_threads & 1) break;
                } else if (current_thread_ID == 1) {
                  if (bitmap_for_threads & 2) break;
                } else if (current_thread_ID == 2) {
                  if (bitmap_for_threads & 4) break;
                } else if (current_thread_ID == 3) {
                  if (bitmap_for_threads & 8) break;
                }



        } while (1);

        RESTORESTATE;
    EA = 1;
    }
}


/*
 * ThreadExit() is called by the thread's own code to terminate
 * itself.  It will never return; instead, it switches context
 * to another thread.
 */
void ThreadExit(void) {
        /*
         * clear the bit for the current thread from the
         * bit mask, decrement thread count (if any),
         * and set current thread to another valid ID.
         * Q: What happens if there are no more valid threads?
         */
        __critical{
        EA = 0;
        RESTORESTATE;
        EA = 1;
        }
}

void myTimer0Handler(){


    EA = 0;
    SAVESTATE;

    __asm
        MOV A, R0
        PUSH ACC
        MOV A, R1
        PUSH ACC
        MOV A, R2
        PUSH ACC
        MOV A, R3
        PUSH ACC
        MOV A, R4
        PUSH ACC
        MOV A, R5
        PUSH ACC
        MOV A, R6
        PUSH ACC
        MOV A, R7
        PUSH ACC
    __endasm;


    do {
            /*
                * @@@ [8 pts] do round-robin policy for now.
                * find the next thread that can run and 
                * set the current thread ID to it,
                * so that it can be restored (by the last line of 
                * this function).
                * there should be at least one thread, so this loop
                * will always terminate.
                */


            current_thread_ID++;
            if (current_thread_ID >= MAXTHREADS) current_thread_ID = 0;

            if (current_thread_ID == 0) {
                if (bitmap_for_threads & 1) break;
            } else if (current_thread_ID == 1) {
                if (bitmap_for_threads & 2) break;
            } else if (current_thread_ID == 2) {
                if (bitmap_for_threads & 4) break;
            } else if (current_thread_ID == 3) {
                if (bitmap_for_threads & 8) break;
            }



    } while (1);

    __asm
        POP ACC
        MOV R7, A
        POP ACC
        MOV R6, A
        POP ACC
        MOV R5, A
        POP ACC
        MOV R4, A
        POP ACC
        MOV R3, A
        POP ACC
        MOV R2, A
        POP ACC
        MOV R1, A
        POP ACC
        MOV R0, A
    __endasm;  

    RESTORESTATE;
    EA = 1;
    
    __asm
        RETI
    __endasm;
}
