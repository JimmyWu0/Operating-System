/* 
 * file: testpreempt.c 
 */
#include <8051.h>
#include "preemptive.h"
#include "buttonlib.h"
#include "keylib.h"
#include "lcdlib.h"


/* 
 * @@@ [2pt] 
 * declare your global variables here, for the shared buffer 
 * between the producer and consumer.  
 * Hint: you may want to manually designate the location for the 
 * variable.  you can use
 *        __data __at (0x30) type var; 
 * to declare a variable var of the type
*/

__data __at (0x35) ThreadID currentThreadID;
__data __at (0x36) char mutex;
__data __at (0x37) char full;
__data __at (0x38) char empty;
__data __at (0x3D) char buffer;
__data __at (0x3E) char last;
__data __at (0x24) char ok1;
__data __at (0x25) char ok2;





void Producer1(void) {
        /*
         * @@@ [2 pt]
         * initialize producer data structure, and then enter
         * an infinite loop (does not return)
         */

        /* @@@ [6 pt]
        * wait for the buffer to be available, 
        * and then write the new data into the buffer
        */
        while (1) {
                SemaphoreWait(ok1);
                //while(!AnyButtonPressed());
                SemaphoreWait(empty);
                while(!AnyButtonPressed());
                SemaphoreWait(mutex);
                if (AnyButtonPressed()) {
	                buffer= ButtonToChar();
	            }
                SemaphoreSignal(mutex);
                while(!AnyButtonPressed());
                SemaphoreSignal(full);
                //while(!AnyButtonPressed());
                SemaphoreSignal(ok2);
        }
}

/* [8 pts] for this function
 * the producer in this test program generates one characters at a
 * time from 'A' to 'Z' and starts from 'A' again. The shared buffer
 * must be empty in order for the Producer to write.
 */
void Producer2(void) {
        /*
         * @@@ [2 pt]
         * initialize producer data structure, and then enter
         * an infinite loop (does not return)
         */
        Init_Keypad();
        /* @@@ [6 pt]
        * wait for the buffer to be available, 
        * and then write the new data into the buffer
        */
        while (1) {
                SemaphoreWait(ok2);
                //while(!AnyKeyPressed());
                SemaphoreWait(empty);
                while(!AnyKeyPressed());
                SemaphoreWait(mutex);
                 if (AnyKeyPressed()) {
	                buffer= KeyToChar();
	            }
                SemaphoreSignal(mutex);
                while(!AnyKeyPressed());
                SemaphoreSignal(full);
                //while(!AnyKeyPressed());
                SemaphoreSignal(ok1);
        }       
}



/* [10 pts for this function]
 * the consumer in this test program gets the next item from
 * the queue and consume it and writes it to the serial port.
 * The Consumer also does not return.
 */
void Consumer(void) {
        /* @@@ [2 pt] initialize Tx for polling */

        /* @@@ [2 pt] wait for new data from producer
        * @@@ [6 pt] write data to serial port Tx, 
        * poll for Tx to finish writing (TI),
        * then clear the flag
        */
        LCD_Init();
        while (1) {
                SemaphoreWait(full);
                SemaphoreWait(mutex);
                if (LCD_ready() && last != buffer) 
                {       
		        LCD_write_char(buffer);
                        last = buffer;
		}
                SemaphoreSignal(mutex);
                SemaphoreSignal(empty);
        }
}

/* [5 pts for this function]
 * main() is started by the thread bootstrapper as thread-0.
 * It can create more thread(s) as needed:
 * one thread can acts as producer and another as consumer.
 */
void main(void) {
        /* 
        * @@@ [1 pt] initialize globals 
        * @@@ [4 pt] set up Producer and Consumer.
        * Because both are infinite loops, there is no loop
        * in this function and no return.
        */
        SemaphoreCreate(&mutex, 1);
        SemaphoreCreate(&full, 0);
        SemaphoreCreate(&empty, 3);
        SemaphoreCreate(&ok1, 1);
        SemaphoreCreate(&ok2, 0);
        currentThreadID = ThreadCreate(Producer1);
        currentThreadID = ThreadCreate(Producer2);
        __asm
                MOV  0x35, #48
                MOV  sp, 0x30
        __endasm;
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
                ljmp  _myTimer0Handler
        __endasm;
}
