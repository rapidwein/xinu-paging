/* control_reg.c - read_cr0 read_cr2 read_cr3 read_cr4
		   write_cr0 write_cr3 write_cr4 enable_pagine */

#include <conf.h>
#include <kernel.h>

unsigned long tmp;


/*-------------------------------------------------------------------------
 * read_cr0 - read CR0
 *-------------------------------------------------------------------------
 */
unsigned long read_cr0(void) {

  STATWORD ps;
  unsigned long local_tmp;

  disable(ps);

  asm("pushl %eax");
  asm("movl %cr0, %eax");
  asm("movl %eax, tmp");
  asm("popl %eax");

  local_tmp = tmp;

  restore(ps);

  return local_tmp;
}

/*-------------------------------------------------------------------------
 * read_cr2 - read CR2
 *-------------------------------------------------------------------------
 */

unsigned long read_cr2(void) {

  STATWORD ps;
  unsigned long local_tmp;

  disable(ps);

  asm("pushl %eax");
  asm("movl %cr2, %eax");
  asm("movl %eax, tmp");
  asm("popl %eax");

  local_tmp = tmp;

  restore(ps);

  return local_tmp;
}


/*-------------------------------------------------------------------------
 * read_cr3 - read CR3
 *-------------------------------------------------------------------------
 */

unsigned long read_cr3(void) {

  STATWORD ps;
  unsigned long local_tmp;

  disable(ps);

  asm("pushl %eax");
  asm("movl %cr3, %eax");
  asm("movl %eax, tmp");
  asm("popl %eax");

  local_tmp = tmp;

  restore(ps);

  return local_tmp;
}


/*-------------------------------------------------------------------------
 * read_cr4 - read CR4
 *-------------------------------------------------------------------------
 */

unsigned long read_cr4(void) {

  STATWORD ps;
  unsigned long local_tmp;

  disable(ps);

  asm("pushl %eax");
  asm("movl %cr4, %eax");
  asm("movl %eax, tmp");
  asm("popl %eax");

  local_tmp = tmp;

  restore(ps);

  return local_tmp;
}


/*-------------------------------------------------------------------------
 * write_cr0 - write CR0
 *-------------------------------------------------------------------------
 */

void write_cr0(unsigned long n) {

  STATWORD ps;

  disable(ps);
  tmp = n;
  asm("pushl %eax");
  //kprintf("line1");
  asm("movl tmp, %eax");		/* mov (move) value at tmp into %eax register. 
					   "l" signifies long (see docs on gas assembler)	*/
  //kprintf("line2");
  asm("movl %eax, %cr0");
  //kprintf("line3");
  asm("popl %eax");
  //kprintf("line4");

  restore(ps);

}


/*-------------------------------------------------------------------------
 * write_cr3 - write CR3
 *-------------------------------------------------------------------------
 */

void write_cr3(unsigned long n) {


  STATWORD ps;

  disable(ps);

  tmp = n;
  asm("pushl %eax");
  asm("movl tmp, %eax");                /* mov (move) value at tmp into %eax register.
                                           "l" signifies long (see docs on gas assembler)       */
  asm("movl %eax, %cr3");
  asm("popl %eax");

  restore(ps);

}


/*-------------------------------------------------------------------------
 * write_cr4 - write CR4
 *-------------------------------------------------------------------------
 */

void write_cr4(unsigned long n) {


  STATWORD ps;

  disable(ps);

  tmp = n;
  asm("pushl %eax");
  asm("movl tmp, %eax");                /* mov (move) value at tmp into %eax register.
                                           "l" signifies long (see docs on gas assembler)       */
  asm("movl %eax, %cr4");
  asm("popl %eax");

  restore(ps);

}


/*-------------------------------------------------------------------------
 * enable_paging - enable paging 
 *-------------------------------------------------------------------------
 */
void enable_paging(){
  
  unsigned long temp =  read_cr0();
  kprintf("cr3 : %x\n",read_cr3());
  kprintf("cr0 prev: %x",temp);
  temp = temp | ( 0x1 << 31 ) | 0x1;
  write_cr0(temp); 
  kprintf("cr0 after:%x",temp);
}


