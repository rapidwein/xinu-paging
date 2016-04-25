/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
SYSCALL	vfreemem(block, size)
	struct	mblock	*block;
	unsigned size;
{
	unsigned int vmaxaddr, vminaddr, top;
	struct mblock *p, *q;
	struct pentry *pptr = &proctab[currpid];
	vmaxaddr = (unsigned int)(pptr->vhpno + pptr->vhpnpages)*NBPG;
	vminaddr = (unsigned int)(pptr->vhpno)*NBPG;
	if (size==0 || size > 128*NBPG || (unsigned)block>(unsigned)vmaxaddr
            || ((unsigned)block)<((unsigned) vminaddr))
                return(SYSERR);
        size = (unsigned)roundmb(size);
	STATWORD ps;
        disable(ps);
        for( p=pptr->vmemlist->mnext,q= pptr->vmemlist;
             p != (struct mblock *) NULL && p < block ;
             q=p,p=p->mnext )
                ;
        if (((top=q->mlen+(unsigned)q)>(unsigned)block && q!= pptr->vmemlist) ||
            (p!=NULL && (size+(unsigned)block) > (unsigned)p )) {
		//kprintf("error in memory\n");
                restore(ps);
                return(SYSERR);
        }
        if ( q!= pptr->vmemlist && top == (unsigned)block ){
			//kprintf("q updated with %d",q->mlen);
                        q->mlen += size;
	}
        else {
                block->mlen = size;
                block->mnext = p;
                q->mnext = block;
                q = block;
        }
        if ( (unsigned)( q->mlen + (unsigned)q ) == (unsigned)p) {
		//kprintf("length of p: %d length of q: %d",p->mlen,q->mlen);
                q->mlen += p->mlen;
                q->mnext = p->mnext;
        }
        restore(ps);
        return(OK);
}
