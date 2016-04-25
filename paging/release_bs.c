#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id) {
	STATWORD ps;
  /* release the backing store with ID bs_id */
   if(isbadbid(bs_id)) return SYSERR;
	disable(ps);
   bs_map_t *bs_ptr;
    bs_ptr = &bsm_tab[bs_id];
   if(bs_ptr->bs_status == BSM_UNMAPPED){
	restore(ps);
	 return SYSERR;
	}
   //kprintf("freeing existing %d BS\n",bs_id);
   free_bsm(bs_id);
   //setpdbr(currpid);
  restore(ps);
   return OK;

}

