/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 * vgetmem  --  allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
WORD	*vgetmem(nbytes)
	unsigned nbytes;
{
	STATWORD ps;
        struct  mblock  *p, *q, *leftover;
        disable(ps);
	//kprintf("no. of bytes %u\n", nbytes);
        if (nbytes==0 || nbytes > 128*NBPG || proctab[currpid].vmemlist == (struct mblock *) NULL) {
                restore(ps);
                return( (WORD *)SYSERR);
        }
        nbytes = (unsigned int) roundmb(nbytes);
	//kprintf("vmemlist %d vmemlist->mnext %d",proctab[currpid].vmemlist,proctab[currpid].vmemlist->mnext);
        for (q= proctab[currpid].vmemlist,p=proctab[currpid].vmemlist->mnext;
             p != (struct mblock *) NULL ;
             q=p,p=p->mnext){
		//kprintf("length of p : %d",p->mlen);
                if ( p->mlen == nbytes) {
                        q->mnext = p->mnext;
			//kprintf("returned");
                        restore(ps);
                        return( (WORD *)p );
                } else if ( p->mlen > nbytes ) {
                        leftover = (struct mblock *)( (unsigned)p + nbytes );
                        q->mnext = leftover;
                        leftover->mnext = p->mnext;
                        leftover->mlen = p->mlen - nbytes;
			//kprintf("returned");
                        restore(ps);
                        return( (WORD *)p );
                }
	}
        restore(ps);

        return( (WORD *)SYSERR );
}
