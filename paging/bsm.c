/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
extern struct pentry proctab[];
SYSCALL init_bsm()
{
	STATWORD ps;
	disable(ps);
	bs_map_t *bs_ptr;
	int i,j;
	for(i = 0; i < NBS; i++) {
		bs_ptr = &bsm_tab[i];
		bs_ptr->bs_status = BSM_UNMAPPED;
		bs_ptr->bs_pid = UNDEFINED;
		bs_ptr->bs_npages = UNDEFINED;
		bs_ptr->bs_vpno = UNDEFINED;
		bs_ptr->bs_sem = UNDEFINED;
		bs_ptr->bs_private = SHARED;
		//bs_ptr->bs_share_count = 0;
		//for(j = 0; j < NPROC; j++)
		//	bs_ptr->bs_sharer[j] = 0;
	}
	restore(ps);
	return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm()
{
	int i;
        bs_map_t *bs_ptr;
        for(i = 0; i < NBS; i++) {
                bs_ptr = &bsm_tab[i];
		if(bs_ptr->bs_status == BSM_UNMAPPED){
			//kprintf("returning bsm with id %d\n", i);
			return i;
		}
	}
	return UNDEFINED;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
	bs_map_t *bs_ptr, *bs_ptr_1;
	struct pentry *pptr;
	int j;
	if(isbadbid(i)) return SYSERR;
	for(j = 0; j < NPROC; j++)
	{
		pptr = &proctab[j];
		if(pptr->pstate != PRFREE && j != currpid)
		{
			bs_ptr_1 = &proctab[j].loc_bsm[i];
			if(bs_ptr_1->bs_status == BSM_MAPPED)
			{	//kprintf("shared by process %d\n",j);
				return SYSERR;
			}
		}
	}
	//if(bs_ptr->bs_share_count != 0) return SYSERR;
	bs_ptr = &bsm_tab[i];
	bs_ptr->bs_status = BSM_UNMAPPED;
	bs_ptr->bs_pid = UNDEFINED;
	bs_ptr->bs_vpno = UNDEFINED;
	bs_ptr->bs_npages = 0;
	bs_ptr->bs_sem = UNDEFINED;
	bs_ptr->bs_private = SHARED;
	pptr = &proctab[currpid];
	bs_ptr_1 = &pptr->loc_bsm[i];
        bs_ptr_1->bs_status = BSM_UNMAPPED;
        bs_ptr_1->bs_pid = UNDEFINED;
        bs_ptr_1->bs_vpno = UNDEFINED;
        bs_ptr_1->bs_npages = 0;
        bs_ptr_1->bs_sem = UNDEFINED;
        bs_ptr_1->bs_private = SHARED;
	return OK;
}
	
/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, unsigned long vaddr, int* store, int* pageth)
{
	//kprintf("lookup");
	int i;
	int vpno = vaddr/NBPG;
	int rem_vpno;
	for(i = 0; i < NBS; i++)
	{
		//kprintf("vpno : %d bsvpno[%d] : %d",vpno, i,proctab[pid].loc_bsm[i].bs_vpno);
		if(proctab[pid].loc_bsm[i].bs_status == BSM_MAPPED){
			rem_vpno = vpno - proctab[pid].loc_bsm[i].bs_vpno;
			//kprintf("vpno : %d bsvpno[%d] : %d bsnp : %d",vpno, i, proctab[pid].loc_bsm[i].bs_vpno, proctab[pid].loc_bsm[i].bs_npages);
			if(rem_vpno >= 0 && (rem_vpno < proctab[pid].loc_bsm[i].bs_npages))
			{
				//kprintf("proc vpno : %d store : %d\n",proctab[pid].loc_bsm[i].bs_vpno,i);
				*store = i;
				*pageth = rem_vpno;
				return OK;
			}
		}
	}
	*store = UNDEFINED;
	*pageth = UNDEFINED;
	return SYSERR;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
	//kprintf("pid : %d\n",pid);
	bs_map_t *bs_ptr;
	bs_ptr = &bsm_tab[source];
	if(isbadbid(source)) return SYSERR;
	//kprintf("badbid : %d\n",isbadbid(source));
	if(isbadnp(npages)) return SYSERR;
	//kprintf("badnp : %d\n",isbadnp(npages));
	if(isbadpid(pid)) return SYSERR;
	//kprintf("badpid : %d\n",isbadpid(pid));
	if(bs_ptr->bs_status == BSM_UNMAPPED)
	{
		bs_ptr->bs_status = BSM_MAPPED;
		bs_ptr->bs_npages = npages;
	//	bs_ptr->bs_vpno = vpno;
		bs_ptr->bs_pid = pid;
	}
	//bs_ptr->bs_share_count++;
	//bs_ptr->bs_sharer[pid] = 1;
/*	struct pentry *pptr = &proctab[pid];
	pptr->loc_bsm[source].bs_status = BSM_MAPPED;
	pptr->loc_bsm[source].bs_pid = pid;
	pptr->loc_bsm[source].bs_npages = npages;
        pptr->loc_bsm[source].bs_vpno = vpno;
        pptr->loc_bsm[source].bs_private = SHARED;*/
	return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
	STATWORD ps;
	disable(ps);
	long vaddr;
	int status,store,pageth;
	struct pentry *pptr = &proctab[pid];
	bs_map_t *bs_ptr;
	vaddr = vpno * NBPG;
	status = bsm_lookup(pid, vaddr, &store, &pageth);
	//kprintf("status : %d",status);
	if(status == SYSERR){
		restore(ps);
		return SYSERR;
	}
	bs_ptr = &proctab[pid].loc_bsm[store];
	if(flag == SHARED)
	{
		int rem_vpno = vpno - bs_ptr->bs_vpno;
		while(rem_vpno < bs_ptr->bs_npages)
		{
			//kprintf("vaddr : %x\n",vaddr);
			pd_t *pd_ptr = pptr->pdbr + ((vaddr&0xffc00000)>>22)*sizeof(pd_t);
			//kprintf("pd : %x\n",pd_ptr);
			pt_t *pt_ptr = (pd_ptr->pd_base)*NBPG + ((vaddr&0x003ff000)>>12)*sizeof(pt_t);
			//kprintf("pt : %x\n",pt_ptr);
			int source = pt_ptr->pt_base - FRAME0;
			//kprintf("frame number : %d\n",source);
			fr_map_t *fr_ptr = &frm_tab[source];
			//kprintf("status : %d frpid %d pid %d\n",fr_ptr->fr_status,fr_ptr->fr_pid,pid);
			if(fr_ptr->fr_status == FRM_MAPPED && fr_ptr->fr_pid == pid){
				//kprintf("freeing frame %d for process %d\n",source,pid);
				free_frm(source);
			}
			if(pd_ptr->pd_pres == 0)
				break;
			vaddr += NBPG;
			rem_vpno++;
		}
		
	}
	else 
		pptr->vmemlist = NULL;
	//if(bs_ptr->bs_share_count == 1)
	//{
		//kprintf("unmapping BS %d of process %d\n",store,pid);
		bs_ptr->bs_status = BSM_UNMAPPED;
		bs_ptr->bs_pid = -1;
		bs_ptr->bs_npages = 0;
		bs_ptr->bs_vpno = UNDEFINED;
		bs_ptr->bs_sem = UNDEFINED;
		bs_ptr->bs_private = SHARED;
	//}
	//bs_ptr->bs_share_count--;
	//bs_ptr->bs_sharer[pid] = 0;
	restore(ps);
	return OK;
}
