/*
 * file: testcoop.c
 */
#include <8051.h>
#include "preemptive.h"

/*
 * @@@ [2pt]
 * declare your global variables here, for the shared buffer
 * between the producer and consumer.
 * Hint: you may want to manually designate the location for the
 * variable.  you can use
 *        __data __at (0x30) type var;
 * to declare a variable var of the type
 */

//__data __at (0x39) char shared_buffer;
//__data __at (0x3A) char buffer_is_empty;
__data __at (0x39) char mutex;
__data __at (0x3A) char full;
__data __at (0x3B) char empty;
__data __at (0x3C) char buffer[3];

/* [8 pts] for this function
 * the producer in this test program generates one characters at a
* time from 'A' to 'Z' and starts from 'A' again. The shared buffer
 * must be empty in order for the Producer to write.
 */
void Producer(void) {
        /*
         * @@@ [2 pt]
         * initialize producer data structure, and then enter
         * an infinite loop (does not return)
         */
        __data __at (0x3F) char produced_character = 'A';
        __data __at (0x20) char p_idx = 0x0; 
        /*while (1) {
                // @@@ [6 pt]
                // wait for the buffer to be available,
                // and then write the new data into the buffer 
                if (buffer_is_empty == 1) {
                        __critical{
                        EA = 0;

                        shared_buffer = produced_character;
                        buffer_is_empty = 0;
                        if (produced_character == 'Z') produced_character = 'A';
                        else produced_character++;
                        
                        EA = 1;
                        //ThreadYield();
                        }
                }
                while (buffer_is_empty == 0) {}
        }
        */
        while (1) {
                SemaphoreWait(empty);
                SemaphoreWait(mutex);
                // add the new char to the buffer
                __critical{
                        EA = 0;
                        buffer[p_idx] = produced_character;
                        produced_character = (produced_character == 'Z' ? 'A' : produced_character + 1);
                        p_idx = (p_idx == 0x02 ? 0x00 : p_idx + 1);
                        EA = 1;
                }
                
                SemaphoreSignal(mutex);
                SemaphoreSignal(full);
        }
}

/* [10 pts for this function]
 * the consumer in this test program gets the next item from
 * the queue and consume it and writes it to the serial port.
 * The Consumer also does not return.
 */
void Consumer(void) {
        /* @@@ [2 pt] initialize Tx for polling */
        TMOD |= 0x20;
        TH1 = -6;
        SCON = 0x50;
        TR1 = 1;

        __data __at (0x21) char c_idx = 0x0; 

        //EA = 1;

        /*while (1) {
                EA = 1;
                // @@@ [2 pt] wait for new data from producer
                // @@@ [6 pt] write data to serial port Tx,
                // poll for Tx to finish writing (TI),
                // then clear the flag
                 

                // if nothing to consume then yield
                while (buffer_is_empty == 1) {
                        //ThreadYield();
                }
                __critical{
                EA = 0;
                SBUF = shared_buffer;

                buffer_is_empty = 1;
                EA = 1;
                }
                while (!TI) {}
                TI = 0;
        }*/
        EA = 1;
        while (1) {
                SemaphoreWait(full);
                SemaphoreWait(mutex);
                // remove the next char from the buffer
                __critical{
                        EA = 0;
                        SBUF = buffer[c_idx];
                        c_idx = (c_idx == 0x02 ? 0x00 : c_idx + 1);
                        EA = 1;
                }
                SemaphoreSignal(mutex);
                SemaphoreSignal(empty);

                while (!TI) {}
                TI = 0;
        }
}

/* [5 pts for this function]
 * main() is started by the thread bootstrapper as thread-0.
 * It can create more thread(s) as needed:
 * one thread can act as producer and another as consumer.
 */
void main(void) {
          /*
           * @@@ [1 pt] initialize globals
           * @@@ [4 pt] set up Producer and Consumer.
           * Because both are infinite loops, there is no loop
           * in this function and no return.
           */
          //shared_buffer = 0;
          //buffer_is_empty = 1;
          SemaphoreCreate(full, 0);
          SemaphoreCreate(mutex, 1);
          SemaphoreCreate(empty, 3);

          ThreadCreate(Producer); //007D
          Consumer();
}

void _sdcc_gsinit_startup(void) {
        __asm
                ljmp  _Bootstrap
        __endasm;
}

void _mcs51_genRAMCLEAR(void) {}
void _mcs51_genXINIT(void) {}
void _mcs51_genXRAMCLEAR(void) {}
void timer0_ISR(void) __interrupt(1) {
        __asm
                ljmp _myTimer0Handler
        __endasm;
}
