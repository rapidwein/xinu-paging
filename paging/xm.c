/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  /* sanity check ! */

  if ( (virtpage < 4096) || isbadbid(source) || isbadnp(npages)){
	kprintf("xmmap call error: parameter error! \n");
	return SYSERR;
  }
  STATWORD ps;
  disable(ps);
  bs_map_t *bs_ptr;
  bs_ptr = &bsm_tab[source];
  if(bs_ptr->bs_status == BSM_UNMAPPED)
  {
	kprintf("unmapped bs\n");
  	restore(ps);
	return SYSERR;
  }
  if(bs_ptr->bs_private == PRIVATE)
  {
	//kprintf("access to private heap\n");
        restore(ps);
        return SYSERR;
  }
  if(bs_ptr->bs_npages < npages)
  {
	//kprintf("insufficient npages : %d\n",bs_ptr->bs_npages);
        restore(ps);
        return SYSERR;
  }
  
  int status = bsm_map(currpid, virtpage, source, npages);
  //kprintf("xmmap returned with status %d", status);
  if(status == SYSERR)
  {
	restore(ps);
  	return SYSERR;
  }
  struct pentry *pptr = &proctab[currpid];
  if(pptr->loc_bsm[source].bs_status == BSM_UNMAPPED)
  	pptr->loc_bsm[source].bs_status = BSM_MAPPED;
  pptr->loc_bsm[source].bs_pid = currpid;
  pptr->loc_bsm[source].bs_npages = npages;
  pptr->loc_bsm[source].bs_vpno = virtpage;
  pptr->loc_bsm[source].bs_private = SHARED;
  restore(ps);
  return OK;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage )
{
  /* sanity check ! */
  if ( (virtpage < 4096) ){ 
	kprintf("xmummap call error: virtpage (%d) invalid! \n", virtpage);
	return SYSERR;
  }
  STATWORD ps;
  disable(ps);
  //kprintf("unmapping for process %d virtpage : %x",currpid,virtpage);
  int status = bsm_unmap(currpid, virtpage, SHARED);
  //kprintf("finished xmunmapping with status %d\n",status);
  if(status == SYSERR)
  {
	restore(ps);
	return SYSERR;
  }
  restore(ps);
  return OK;
}

