/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
  int i;
  fr_map_t *fr_ptr;
  for(i = 0; i < NFRAMES; i++)
  {
    fr_ptr = &frm_tab[i];
    fr_ptr->fr_status = FRM_UNMAPPED;
    fr_ptr->fr_pid = UNDEFINED;
    fr_ptr->fr_vpno = UNDEFINED;
    fr_ptr->fr_refcnt = 0;
    fr_ptr->fr_type = UNDEFINED;
    fr_ptr->fr_dirty = UNDEFINED;
    fr_ptr->cookie = NULL;
    fr_ptr->fr_loadtime = UNDEFINED;
  }
  return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm()
{
  int i, j, k,policy;
  fr_map_t *fr_ptr;
 // fifo_t *curr;
  /*for(i = 0; i < NBS; i++)
  {
	bs_map_t *bs_ptr = &bsm_tab[i];
	if(bs_ptr->bs_share_count > 1 && bs_ptr->bs_sharer[currpid] == 1)
	{
		for(j = 0; j < NPROC; j++)
		{
			if(j != currpid && bs_ptr->bs_sharer[j] == 1)
				for(k = 0; k < NFRAMES; k++)
				{
					if(proctab[j].loc_frm[k] == 1 && frm_tab[k].fr_status == FRM_MAPPED && frm_tab[k].fr_type == FR_PAGE && proctab[currpid].loc_frm[k] == 0)
						return k;
				}
		}
	}
  }*/
  for(i = 0; i < NFRAMES; i++)
  {
    fr_ptr = &frm_tab[i];
    if(fr_ptr->fr_status == FRM_UNMAPPED)
    {
	//kprintf("returning frame %d\n",i);
      return i;
    }
  }
  //kprintf("no free frame found : i : %d\n",i);
  if(i == NFRAMES)
  {
    // No free frames found
    policy = grpolicy();
    //kprintf("fifo");
    if(policy == FIFO)
    {
      fifo_t *fifo_ptr = &fifo_head;
	fifo_ptr = fifo_ptr->fr_next;
      if(fifo_ptr){
      	free_frm(fifo_ptr->fr_id);
      	kprintf("replaced frame %d\n",FRAME0+fifo_ptr->fr_id); 
	fifo_head.fr_next = fifo_ptr->fr_next;
      	return fifo_ptr->fr_id;
	}
    }
    else if(policy == LRU)
    {
	int k,min = 65536, min_id = 0,max_vpno = 0;
	for(k = 0; k < NFRAMES; k++)
	{
		fr_map_t *fr_tmp = &frm_tab[k];
		if(fr_tmp->fr_loadtime <= min && fr_tmp->fr_status == FRM_MAPPED && fr_tmp->fr_loadtime != -1 && fr_tmp->fr_type == FR_PAGE){
			if(fr_tmp->fr_loadtime == min){
				if(fr_tmp->fr_vpno > max_vpno){
					max_vpno = fr_tmp->fr_vpno;
					min_id = k;
				}	
			}
			else{
				min = fr_tmp->fr_loadtime;
				min_id = k;
			}
		}
	}
	free_frm(min_id);
	kprintf("replaced frame %d\n",FRAME0+min_id);
	return min_id;
    }
  }
  return UNDEFINED;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
  long vaddr;
  int store, pageth, status;
  if(isbadfid(i))
    return SYSERR;
  fr_map_t *fr_ptr = &frm_tab[i];
  struct pentry *pptr = &proctab[frm_tab[i].fr_pid];
  //kprintf("freeing the frame %d of type %d\n",i,fr_ptr->fr_type,FR_PAGE);
  if(fr_ptr->fr_type == FR_PAGE && fr_ptr->fr_status == FRM_MAPPED){
	int j,k;
  vaddr = fr_ptr->fr_vpno*NBPG;
  //kprintf("freeing the page %d\n",i);
  	status = bsm_lookup(fr_ptr->fr_pid, vaddr, &store, &pageth);
	//kprintf("returned with status %d for pid %d vaddr %x\n",status,fr_ptr->fr_pid, vaddr);
   	if(status == SYSERR) return SYSERR;
	pd_t *pd_ptr = (pd_t*)(pptr->pdbr + ((vaddr&0xffc00000)>>22)*sizeof(pd_t));
	//kprintf("pdbr : %x pd : %x\n",pptr->pdbr,pd_ptr);
	pt_t *pt_ptr = (pt_t*)(pd_ptr->pd_base*NBPG + ((vaddr&0x003ff000)>>12)*sizeof(pt_t));
	//kprintf("pd_base : %x pt : %x\n",pd_ptr->pd_base,pt_ptr);
	//kprintf("vaddr : %x store : %d page : %d\n",vaddr,store,pageth);
	write_bs(vaddr,store,pageth);
	fr_ptr->fr_refcnt--;
	proctab[fr_ptr->fr_pid].loc_frm[i] = 0;
	if(fr_ptr->fr_refcnt <= 0)
	{
		fifo_t *tmp = &fifo_head;
		while(tmp)
		{
			fifo_t *curr = tmp;
			tmp = tmp->fr_next;
			if(tmp && tmp->fr_id == i)
				curr->fr_next = tmp->fr_next;
			//kprintf("deleted fifo node %d\n",i);
		}
		fr_ptr->fr_status = FRM_UNMAPPED;
		fr_ptr->fr_pid = UNDEFINED;
    		fr_ptr->fr_vpno = UNDEFINED;
    		fr_ptr->fr_refcnt = 0;
    		fr_ptr->fr_type = UNDEFINED;
    		fr_ptr->fr_dirty = UNDEFINED;
	    	fr_ptr->cookie = NULL;
	    	fr_ptr->fr_loadtime = UNDEFINED;
		//kprintf("unmapped frame %d\n",i);
	}
	int pt_frm = pd_ptr->pd_base - FRAME0;
	//kprintf("page table frame of entry %d: %x\n",pt_frm,pt_ptr);
	fr_ptr = &frm_tab[pt_frm];
	fr_ptr->fr_refcnt--;
	pt_ptr -> pt_pres = 0;
	pt_ptr -> pt_write = 1;
	pt_ptr -> pt_user = 0;
	pt_ptr -> pt_pwt = 0;
	pt_ptr -> pt_pcd = 0;
	pt_ptr -> pt_acc = 0;
	pt_ptr -> pt_dirty = 0;
	pt_ptr -> pt_mbz = 0;
	pt_ptr -> pt_global = 0;
	pt_ptr -> pt_avail = 0;
	pt_ptr -> pt_base = 0;
	if(fr_ptr->fr_refcnt <= 0){
		//kprintf("freeing page 10 from page 11\n");
		free_frm(pt_frm);
	}
   	return OK;
  }
  else if(fr_ptr->fr_type == FR_TBL && fr_ptr->fr_status == FRM_MAPPED){
	//vaddr = (FRAME0 + i)*NBPG;
  	//vaddr = fr_ptr->fr_vpno*NBPG;
        //status = bsm_lookup(fr_ptr->fr_pid, vaddr, &store, &pageth);
	//kprintf("returned with status %d for pid %d vaddr %x\n",status,fr_ptr->fr_pid, vaddr);
        //if(status == SYSERR) return SYSERR;
	int j;
	vaddr = (FRAME0 + i)*NBPG;
		//kprintf("faulted address : %x\n",(pptr->pdbr + ((vaddr&0xffc00000)>>22)*sizeof(pd_t)));
	pd_t *pd_ptr = (pd_t*)(pptr->pdbr);
	for(j = 0; j < 1024; j++)
	{
		//kprintf("faulted address : %x\n",(vaddr + j*sizeof(pt_t)));
		pt_t *pt_ptr = (pt_t*)(vaddr + j*sizeof(pt_t));
		int pg_frm = pt_ptr->pt_base - FRAME0;
		if(pt_ptr->pt_pres == 1){
			//kprintf("freeing entry(%d) %x\n",j,pt_ptr);
			free_frm(pg_frm);
		}
	}
	//kprintf("freed all entries\n");
	fr_ptr->fr_refcnt--;
		proctab[fr_ptr->fr_pid].loc_frm[i] = 0;
	if(fr_ptr->fr_refcnt <= 0)
	{
		fr_ptr->fr_status = FRM_UNMAPPED;
                fr_ptr->fr_pid = UNDEFINED;
                fr_ptr->fr_vpno = UNDEFINED;
                fr_ptr->fr_refcnt = 0;
                fr_ptr->fr_type = UNDEFINED;
                fr_ptr->fr_dirty = UNDEFINED;
                fr_ptr->cookie = NULL;
                fr_ptr->fr_loadtime = UNDEFINED;
	}
	//kprintf("unmapped page table\n");
	int pd_frm = pd_ptr->pd_base - FRAME0;
	while(pd_frm != i){
		pd_ptr++;
		pd_frm = pd_ptr->pd_base - FRAME0;
	}
	//fr_ptr = &frm_tab[pd_frm];
        //fr_ptr->fr_refcnt--;
        pd_ptr -> pd_pres = 0;
        pd_ptr -> pd_write = 1;
        pd_ptr -> pd_user = 0;
        pd_ptr -> pd_pwt = 0;
        pd_ptr -> pd_pcd = 0;
        pd_ptr -> pd_acc = 0;
        pd_ptr -> pd_fmb = 0;
        pd_ptr -> pd_mbz = 0;
        pd_ptr -> pd_global = 0;
        pd_ptr -> pd_avail = 0;
        pd_ptr -> pd_base = 0;
	//kprintf("unset PD entry\n");
        return OK;
  }
  else if(fr_ptr->fr_type == FR_DIR && fr_ptr->fr_status == FRM_MAPPED){
	vaddr = (FRAME0 + i)*NBPG;
  	//vaddr = fr_ptr->fr_vpno*NBPG;
	//status = bsm_lookup(fr_ptr->fr_pid, vaddr, &store, &pageth);
	//kprintf("pd : returned with status %d for pid %d vaddr %x\n",status,fr_ptr->fr_pid, vaddr);
        //if(status == SYSERR) return SYSERR;
	int j;
	vaddr = (FRAME0 + i)*NBPG;
	for(j = 4; j < 1024; j++)
	{
		pd_t *pd_ptr = (pd_t*)(pptr->pdbr + j*sizeof(pd_t));
		int pt_frm = pd_ptr->pd_base - FRAME0;
		if(pd_ptr->pd_pres == 1){
			//kprintf("freeing existing page table\n");
			free_frm(pt_frm);
		}
	}
	fr_ptr->fr_refcnt--;
		proctab[fr_ptr->fr_pid].loc_frm[i] = 0;
	if(fr_ptr->fr_refcnt <= 0)
	{
		//kprintf("unmapping frame for PD\n");
		fr_ptr->fr_status = FRM_UNMAPPED;
                fr_ptr->fr_pid = UNDEFINED;
                fr_ptr->fr_vpno = UNDEFINED;
                fr_ptr->fr_refcnt = 0;
                fr_ptr->fr_type = UNDEFINED;
                fr_ptr->fr_dirty = UNDEFINED;
                fr_ptr->cookie = NULL;
                fr_ptr->fr_loadtime = UNDEFINED;
	}
		//kprintf("process completed\n");
	return OK;
  }
  return OK;
}
