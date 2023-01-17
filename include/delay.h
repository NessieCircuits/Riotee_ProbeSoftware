#ifndef __DELAY_H_
#define __DELAY_H_

/* Delay execution by a number of CPU cycles */
/* Delay execution by a number of CPU cycles */
__attribute__((naked)) static void __delay_cycles(unsigned int cycles) {
  asm volatile("loop1: sub r0, #3\n" // 1 cycle
               "bgt   loop1\n"       // 2 cycles if loop taken, 1 if not
               "bx lr\n");
}

#endif /* __DELAY_H_ */