#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>
SYSCALL ptcreate(int pid)
{
	int i, source = get_frm();
	pt_t *entry;
	if(isbadfid(source))
		return UNDEFINED;
	fr_map_t *fr_ptr;
	fr_ptr = &frm_tab[source];
	fr_ptr->fr_status = FRM_MAPPED;
	fr_ptr->fr_pid = pid;
	fr_ptr->fr_type = FR_TBL;
	proctab[pid].loc_frm[source] = 1;
	for(i = 0; i < 1024; i++) // no. of entries in a page table
	{
		entry = ((FRAME0 + source)*NBPG + (i*sizeof(pt_t)));
		entry->pt_pres = 0;
		entry->pt_write = 1;
		entry->pt_user = 0;
		entry->pt_pwt = 0;
		entry->pt_pcd = 0;
		entry->pt_acc = 0;
		entry->pt_dirty = 0;
		entry->pt_mbz = 0;
		entry->pt_global = 0;
		entry->pt_avail = 0;
		entry->pt_base = 0;
	}
	return source;
}

SYSCALL pdcreate(int pid)
{
        int i, source = get_frm();
	//kprintf("pd for proc %d : %d\n",pid,source);
	pd_t *entry;
        if(isbadfid(source))
                return SYSERR;
        fr_map_t *fr_ptr;
        fr_ptr = &frm_tab[source];
        fr_ptr->fr_status = FRM_MAPPED;
        fr_ptr->fr_pid = pid;
        fr_ptr->fr_type = FR_DIR;
	proctab[pid].loc_frm[source] = 1;
	for(i = 0; i < 1024; i++) // no. of entries in a page table
        {
                entry = ((FRAME0 + source)*NBPG + (i*sizeof(pd_t)));
                entry->pd_pres = 0;
                entry->pd_write = 1;
                entry->pd_user = 0;
                entry->pd_pwt = 0;
                entry->pd_pcd = 0;
                entry->pd_acc = 0;
                entry->pd_mbz = 0;
		entry->pd_fmb = 0;
                entry->pd_global = 0;
                entry->pd_avail = 0;
                entry->pd_base = 0;
        }
	pd_t *ptr = (pd_t *)proctab[NULLPROC].pdbr;
	for(i = 0; i < 4; i++,ptr++)
	{
                entry = ((FRAME0 + source)*NBPG + (i*sizeof(pd_t)));
		entry->pd_pres = 1;
		//kprintf("entry present : %x\n",entry);
		//entry->pd_base = pt_global[i];
		entry->pd_base = i + FRAME0;
		//entry->pd_base = ptr->pd_base;
		//kprintf("pt_global[%d]=%d\n",i,pt_global[i]);
		fr_ptr->fr_refcnt++;	
		//kprintf("0x%x->ptbase = %d\n",entry,entry->pd_base);	
	}
	proctab[pid].pdbr = (FRAME0 + source)*NBPG;
	//kprintf("pdbr in pdcreate = 0x%x\n",proctab[pid].pdbr);
	//kprintf("%d is a PD\n",source);
        return source;
}

SYSCALL setpdbr(int pid)
{
	int pdbr_no;
	pdbr_no = (proctab[pid].pdbr/NBPG - FRAME0);
	//kprintf("pdbr = 0x%x\n",proctab[pid].pdbr);
	fr_map_t *fr_ptr = &frm_tab[pdbr_no];
	if(fr_ptr->fr_status == FRM_UNMAPPED || fr_ptr->fr_pid != pid || fr_ptr->fr_type != FR_DIR) return SYSERR;
	write_cr3(proctab[pid].pdbr&0xFFFFF000);
}
SYSCALL globalpt()
{
	int i, j, pt_index;
	pt_t *entry;
	for(i = 0; i < 4; i++)
	{
		pt_index = ptcreate(NULLPROC);
		if(pt_index == SYSERR) return SYSERR;
		pt_global[i] = (pt_index + FRAME0);
		for(j = 0; j < 1024; j++) // no. of entries in a page table
		{
			entry = (pt_global[i]*NBPG + j*sizeof(pt_t));
			entry->pt_pres = 1;
			entry->pt_base = i*1024 + j;
		}
		frm_tab[pt_index].fr_refcnt++;
	}
	return OK;
}

