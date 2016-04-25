#include <conf.h>
#include <kernel.h>
#include <mark.h>
#include <bufpool.h>
#include <proc.h>
#include <paging.h>

SYSCALL read_bs(char *dst, bsd_t bs_id, int page) {

  /* fetch page page from map map_id
     and write beginning at dst.
  */
   if(isbadbid(bs_id)) return SYSERR;
   if(page < 0 && isbadnp(page)) return SYSERR;
   void * phy_addr = BACKING_STORE_BASE + BACKING_STORE_UNIT_SIZE*bs_id + page*NBPG;
   char *tmp = BACKING_STORE_BASE + BACKING_STORE_UNIT_SIZE*bs_id + page*NBPG;
   //kprintf("reading from addr %x",tmp);
   //kprintf("reading data %c",*tmp);
   bcopy(phy_addr, (void*)dst, NBPG);
   //kprintf("copied addr : %x\n",dst);
   //kprintf("copied data : %c\n",*dst);
}


