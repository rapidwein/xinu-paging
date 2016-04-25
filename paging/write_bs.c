#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <mark.h>
#include <bufpool.h>
#include <paging.h>

int write_bs(char *src, bsd_t bs_id, int page) {

  /* write one page of data from src
     to the backing store bs_id, page
     page.
  */
   if(isbadbid(bs_id)) return SYSERR;
   if(page < 0 && isbadnp(page)) return SYSERR;
   char * phy_addr = BACKING_STORE_BASE + BACKING_STORE_UNIT_SIZE*bs_id + page*NBPG;
   //kprintf("writing page to backing store");
   bcopy((void*)src, phy_addr, NBPG);
   //kprintf("written addr : %x\n",src);
   //kprintf("written data : %c\n",*src);
   //kprintf("copied addr : %x\n",phy_addr);
   //kprintf("written data : %c\n",*src);
}

