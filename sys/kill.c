/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev, i;

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	for(i = 0; i < NBS; i++)
	{
		bs_map_t *bs_ptr = &pptr->loc_bsm[i];
		if(bs_ptr->bs_status == BSM_MAPPED){
			bsm_unmap(pid,bs_ptr->bs_vpno, bs_ptr->bs_private);
		}
		free_bsm(i);
	}
	int source = pptr->pdbr/NBPG - FRAME0;
	free_frm(source);
	for(i = 0; i < NFRAMES; i++)
	{
		if(frm_tab[i].fr_pid == pid && frm_tab[i].fr_status == FRM_MAPPED){
			//kprintf("freeing the frame %d for pid %d\n",i,pid);
			frm_tab[i].fr_status = FRM_UNMAPPED;
			frm_tab[i].fr_pid = UNDEFINED;
			frm_tab[i].fr_type = UNDEFINED;
			frm_tab[i].fr_refcnt = 0;
			frm_tab[i].fr_loadtime = UNDEFINED;
			frm_tab[i].fr_vpno = UNDEFINED;
			fifo_t *tmp = &fifo_head,*curr;
        		while(tmp){
		                curr = tmp;
                		tmp = tmp->fr_next;
		                /*if(tmp && tmp->fr_id == -1)
                		        curr->fr_next = tmp->fr_next;*/
		                if(tmp && tmp->fr_id == i){
                		        curr->fr_next = tmp->fr_next;
		                        //tmp->fr_next = NULL;
					break;
                		}
		        }
		}
	}
	fifo_t *tmp = &fifo_head,*curr;
	while(tmp){
		curr = tmp;
		tmp = tmp->fr_next;
		/*if(tmp && tmp->fr_id == -1)
			curr->fr_next = tmp->fr_next;*/
		if(tmp && (pid == frm_tab[tmp->fr_id].fr_pid)){
			curr->fr_next = tmp->fr_next;
			//tmp->fr_next = NULL;
		}
	}
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}
	restore(ps);
	return(OK);
}
