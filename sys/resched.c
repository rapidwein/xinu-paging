/* resched.c  -  resched */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>

unsigned long currSP;	/* REAL sp of current process */

/*------------------------------------------------------------------------
 * resched  --  reschedule processor to highest priority ready process
 *
 * Notes:	Upon entry, currpid gives current process id.
 *		Proctab[currpid].pstate gives correct NEXT state for
 *			current process if other than PRREADY.
 *------------------------------------------------------------------------
 */
int	resched()
{
	STATWORD		PS;
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */
	register int i;

	disable(PS);
	/* no switch needed if current process priority higher than next*/

	if ( ( (optr= &proctab[currpid])->pstate == PRCURR) &&
	   (lastkey(rdytail)<optr->pprio)) {
		restore(PS);
		return(OK);
	}
	
#ifdef STKCHK
	/* make sure current stack has room for ctsw */
	asm("movl	%esp, currSP");
	if (currSP - optr->plimit < 48) {
		kprintf("Bad SP current process, pid=%d (%s), lim=0x%lx, currently 0x%lx\n",
			currpid, optr->pname,
			(unsigned long) optr->plimit,
			(unsigned long) currSP);
		panic("current process stack overflow");
	}
#endif	

	/* force context switch */

	if (optr->pstate == PRCURR) {
		optr->pstate = PRREADY;
		insert(currpid,rdyhead,optr->pprio);
	}
	int old_pid = currpid;
	/* remove highest priority process at end of ready list */

	nptr = &proctab[ (currpid = getlast(rdytail)) ];
	nptr->pstate = PRCURR;		/* mark it currently running	*/
#ifdef notdef
#ifdef	STKCHK
	if ( *( (int *)nptr->pbase  ) != MAGIC ) {
		kprintf("Bad magic pid=%d value=0x%lx, at 0x%lx\n",
			currpid,
			(unsigned long) *( (int *)nptr->pbase ),
			(unsigned long) nptr->pbase);
		panic("stack corrupted");
	}
	/*
	 * need ~16 longs of stack space below, so include that in check
	 *	below.
	 */
	if (nptr->pesp - nptr->plimit < 48) {
		kprintf("Bad SP pid=%d (%s), lim=0x%lx will be 0x%lx\n",
			currpid, nptr->pname,
			(unsigned long) nptr->plimit,
			(unsigned long) nptr->pesp);
		panic("stack overflow");
	}
#endif	/* STKCHK */
#endif	/* notdef */
#ifdef	RTCLOCK
	preempt = QUANTUM;		/* reset preemption counter	*/
#endif
#ifdef	DEBUG
	PrintSaved(nptr);
#endif
	int j,k,m;
	for(j = 0; j < NFRAMES; j++)
	{
		fr_map_t *fr_ptr = &frm_tab[j];
		int store,pageth;
		unsigned long vaddr = fr_ptr->fr_vpno*NBPG;
		//kprintf("Comparing frame status %d and frame pid %d with pid %d\n",fr_ptr->fr_status,fr_ptr->fr_pid,old_pid);
		if(proctab[old_pid].loc_frm[j] == 1)
		{
			//kprintf("found matching frame %d for pid %d\n",j,old_pid);
			if(fr_ptr->fr_type == FR_PAGE)
			{
			//kprintf("found matching page\n");
				int status = bsm_lookup(old_pid, vaddr, &store, &pageth);
				//kprintf("returned with status %d for pid %d vaddr %x in resched\n",status,old_pid, vaddr);
        			if(status != SYSERR)
				{
					pd_t *pd_ptr = optr->pdbr + ((vaddr&0xffc00000)>>22)*sizeof(pd_t);
					pt_t *pt_ptr = pd_ptr->pd_base + ((vaddr&0x003ff000)>>12)*sizeof(pt_t);
					//if(pt_ptr->pt_dirty){ kprintf("writing data by process %d\n",old_pid);
						write_bs(vaddr,store,pageth);
					//}
				}
			}
		}
	}
	setpdbr(currpid);
	//kprintf("switching process from %d to %d\n",old_pid,currpid);
	for(j = 0; j < NBS; j++)
	{
		bs_map_t *bs_ptr = &nptr->loc_bsm[j];
		if(bs_ptr->bs_status == BSM_MAPPED)
		{
			for(k = 0; k < NFRAMES; k++)
			{
				fr_map_t *fr_ptr_1 = &frm_tab[k];
				if(fr_ptr_1->fr_status == FRM_MAPPED && fr_ptr_1->fr_pid == currpid && fr_ptr_1->fr_type == FR_PAGE){
					//kprintf("found frame %d with status %d pid %d type %d vpno %x\n",k,fr_ptr_1->fr_status,fr_ptr_1->fr_pid,fr_ptr_1->fr_type,fr_ptr_1->fr_vpno);
					int store, pageth, status = bsm_lookup(currpid,((fr_ptr_1->fr_vpno)*NBPG), &store, &pageth);
					if(status != SYSERR)
						read_bs((fr_ptr_1->fr_vpno)*NBPG, store, pageth);
				}
			}
		}
	}
	ctxsw(&optr->pesp, optr->pirmask, &nptr->pesp, nptr->pirmask);

#ifdef	DEBUG
	PrintSaved(nptr);
#endif
	
	/* The OLD process returns here when resumed. */
	restore(PS);
	return OK;
}



#ifdef DEBUG
/* passed the pointer to the regs in the process entry */
PrintSaved(ptr)
    struct pentry *ptr;
{
    unsigned int i;

    if (ptr->pname[0] != 'm') return;
    
    kprintf("\nSaved context listing for process '%s'\n",ptr->pname);
    for (i=0; i<8; ++i) {
	kprintf("     D%d: 0x%08lx	",i,(unsigned long) ptr->pregs[i]);
	kprintf("A%d: 0x%08lx\n",i,(unsigned long) ptr->pregs[i+8]);
    }
    kprintf("         PC: 0x%lx",(unsigned long) ptr->pregs[PC]);
    kprintf("  SP: 0x%lx",(unsigned long) ptr->pregs[SSP]);
    kprintf("  PS: 0x%lx\n",(unsigned long) ptr->pregs[PS]);
}
#endif


