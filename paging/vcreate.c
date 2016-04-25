/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/
LOCAL newpid();

/*------------------------------------------------------------------------*/
/*  create  -  create a process to start running a procedure		*/
/*------------------------------------------------------------------------*/
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	STATWORD ps;
	disable(ps);
	int status,source;
	struct pentry *pptr;
	bs_map_t *bs_ptr;
	int pid = create(procaddr,ssize,priority,name,nargs,args);
	pptr = &proctab[pid];
	source = get_bsm();
	if(isbadbid(source))
	{
		kill(pid);
		restore(ps);
		return SYSERR;
	}
	status = bsm_map(pid, 4096, source, hsize);
	if(status == SYSERR)
	{
		kill(pid);
		restore(ps);
		return SYSERR;
	}
	struct mblock *tmp_mem = BACKING_STORE_BASE + BACKING_STORE_UNIT_SIZE*source;
        tmp_mem->mlen = hsize*NBPG;
        tmp_mem->mnext = NULL;
	bs_ptr = &bsm_tab[source];
	pptr->loc_bsm[source].bs_status = BSM_MAPPED;
	pptr->loc_bsm[source].bs_private = PRIVATE;
	pptr->loc_bsm[source].bs_pid = pid;
	pptr->loc_bsm[source].bs_npages = hsize;
	pptr->loc_bsm[source].bs_vpno = 4096;
	pptr->store = source;
	pptr->vhpno = 4096;
	pptr->vhpnpages = hsize;
	pptr->vmemlist = (struct mblock *)getmem(sizeof(struct mblock*));
	pptr->vmemlist->mnext = (struct mblock *)(4096*NBPG);
	pptr->vmemlist->mlen = 0;
	//struct mblock *tmp_mem = BACKING_STORE_BASE + BACKING_STORE_UNIT_SIZE*source;
        //tmp_mem->mlen = hsize*NBPG;
        //tmp_mem->mnext = NULL;
	restore(ps);
	return(pid);
}
/* 
       STATWORD        ps;
	disable(ps);
	int status;
	int source;
	int mlen;
	struct pentry *pptr;
	bs_map_t *bs_ptr;
	kprintf("first creating a normal process\n");
	int pid = create(procaddr,ssize,priority,name,nargs,args);
	kprintf("created normal process\n");
	pptr = &proctab[pid];
	source = get_bsm();
		kprintf("source %d\n",source);
	kprintf("bad bid : %d\n",isbadbid(source));
	if(isbadbid(source)){
		kprintf("invalid source %d\n",source);
	 	restore(ps);
		return SYSERR;
	}
	status = bsm_map(pid, 4096, source, hsize);
	kprintf("status : %d pid %d source %d hsize %d",status,pid,source,hsize);
        if(status == SYSERR){
		kprintf("error mapping BS\n");
		restore(ps);
                return SYSERR;
	}
	kprintf("successfully mapped bs\n");
	bs_ptr = &bsm_tab[source];
        bs_ptr->bs_private = PRIVATE;
	pptr->loc_bsm[source].bs_private = PRIVATE;
        pptr->loc_bsm[source].bs_pid = pid;
        pptr->loc_bsm[source].bs_npages = hsize;
        pptr->loc_bsm[source].bs_vpno = 4096;
	pptr->store = source;
	pptr->vhpno = 4096;
	pptr->vhpnpages = hsize;
	kprintf("trying to get mem\n");
	pptr->vmemlist = getmem(sizeof(struct mblock*));
	kprintf("success getting mem %x\n",pptr->vmemlist);
	pptr->vmemlist->mlen = 0;
	//pptr->vmemlist->mnext = (struct mblock*)roundmb((4096*NBPG));
	struct mblock *tmp = getmem(sizeof(struct mblock*));
	kprintf("success getting mem for tmp : %x\n",tmp);
	tmp = (struct mblock*)(4096*NBPG);
	kprintf("success getting mem for tmp : %x\n",tmp);
	//tmp->mlen = (hsize*NBPG);
	kprintf("tmp->len : %d\n",tmp->mlen);
	tmp->mnext = NULL;
	kprintf("tmp->mnext : %x\n",tmp->mnext);
	pptr->vmemlist->mnext = tmp;
	//pptr->vmemlist->mnext->mlen = hsize*NBPG;
	kprintf("created virtual process \n");
	pptr->loc_bsm[source].bs_status = bs_ptr->bs_status;
	restore(ps);
        return(pid);
}*/
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}
