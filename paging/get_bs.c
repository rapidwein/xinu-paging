#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) {
    
  /* requests a new mapping of npages with ID map_id */
    if(isbadbid(bs_id) || isbadnp(npages)) return SYSERR;
    STATWORD ps;
    disable(ps);
    bs_map_t *bs_ptr;
    bs_ptr = &bsm_tab[bs_id];
    if(bs_ptr->bs_status == BSM_UNMAPPED)
    {
	int status = bsm_map(currpid, 4096, bs_id, npages);
	restore(ps);
	if(status == SYSERR) return SYSERR;
	struct pentry *pptr = &proctab[currpid];
  pptr->loc_bsm[bs_id].bs_status = BSM_MAPPED;
        pptr->loc_bsm[bs_id].bs_pid = currpid;
        pptr->loc_bsm[bs_id].bs_npages = npages;
        pptr->loc_bsm[bs_id].bs_vpno = 4096;
        pptr->loc_bsm[bs_id].bs_private = SHARED;
	//kprintf("return %d pages\n",npages);
	return npages;
    }
    restore(ps);
    if(bs_ptr->bs_private == PRIVATE) return SYSERR;
    //kprintf("returning existing %d pages\n",bs_ptr->bs_npages);
    return bs_ptr->bs_npages;
}
