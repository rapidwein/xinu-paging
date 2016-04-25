/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

extern int timeCount;
/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{
  STATWORD ps;
  disable(ps);
  int i, store, pageth, status, source, p, q;
  pd_t *pd;
  pt_t *pt;
  fr_map_t *fr_ptr;
  fifo_t *fifo_ptr = (fifo_t*)getmem(sizeof(fifo_t)), *curr = (fifo_t*)getmem(sizeof(fifo_t));
  /*for(i = 0 ; i < NFRAMES; i++)
  {
    fr_ptr = &frm_tab[i];
    
  }*/
  unsigned long a = read_cr2();
  //kprintf("a : %x : ",a);
  //virt_addr_t *ptr = (virt_addr_t*)a;
  //kprintf("virtaddr %x\n",ptr);
  status = bsm_lookup(currpid, a, &store, &pageth);
  if(status == SYSERR)
  {
	//kprintf("Address %lu is invalid", a);
	kill(currpid);
	restore(ps);
	return SYSERR;
  }
  //kprintf("frame lookup status : %d store : %d pageth : %d\n",status, store, pageth);
  //virt_addr_t *ptr = (virt_addr_t*)a;
  p = ((a&0xffc00000)>>22);
  q = ((a&0x003ff000)>>12);
  //p = ptr->pd_offset;
  //q = ptr->pt_offset;
  //kprintf("p : %d : q : %d : store : %d\n",p,q,store);
  //kprintf("pd : %x", proctab[currpid].pdbr);
  //kprintf("vp : %d\n",a/NBPG);
  pd = (pd_t *)(proctab[currpid].pdbr + p*sizeof(pd_t));
  //kprintf("pd : %x present? %d", pd,pd->pd_pres);
  if(pd->pd_pres == 0)
  {
	
	source = ptcreate(currpid);
	if(source == -1)
	{
		//kprintf("Unable to create page table\n");
		restore(ps);
		return SYSERR;
	}
	pd->pd_pres = 1;
	pd->pd_base = FRAME0 + source;
	//kprintf("Source 1 : %d\n", source);
	source = (unsigned int)pd/NBPG - FRAME0;
	//kprintf("Source 2 : %d\n", source);
	frm_tab[source].fr_refcnt++;
  }
  timeCount++;
  source = get_frm();
  if(source == -1)
  {
	//kprintf("no free frame available\n");
	restore(ps);
	return SYSERR;
  }
  fr_ptr = &frm_tab[source]; 
  fr_ptr->fr_status = FRM_MAPPED;
  fr_ptr->fr_pid = currpid;
  fr_ptr->fr_vpno = a/NBPG;
  fr_ptr->fr_refcnt = 1;
  fr_ptr->fr_type = FR_PAGE;
  proctab[currpid].loc_frm[source] = 1;
  //kprintf("reading bs\n");
  void* dst = (FRAME0+source)*NBPG;
  char *rd = dst;
  read_bs(dst,store, pageth);
  //kprintf("read successful\n");
  //kprintf("read data : %c\n",*rd);
  fifo_ptr = &fifo_head;
  while(fifo_ptr != NULL)
  {
	curr = fifo_ptr;
	fifo_ptr = fifo_ptr->fr_next;
  }
  
  fifo_t *new_node = (fifo_t*)getmem(sizeof(fifo_t));
  if(frm_tab[source].fr_type == FR_PAGE){
  new_node->fr_id = source;
  new_node->fr_next = NULL;
  curr->fr_next = new_node;
  }
  pt = pd->pd_base*NBPG + q*sizeof(pt_t);
  pt->pt_pres = 1;
  int k;
  for(k = 0; k < NFRAMES; k++)
  {
	fr_map_t *fr_tmp = &frm_tab[k];
	if(fr_tmp->fr_status == FRM_MAPPED && fr_tmp->fr_type == FR_PAGE)
	{
		pd_t *pd_tmp = proctab[fr_tmp->fr_pid].pdbr + (((fr_tmp->fr_vpno*NBPG)&0xffc00000)>>22)*sizeof(pd_t);
		pt_t *pt_tmp = (pd_tmp->pd_base)*NBPG + (((fr_tmp->fr_vpno*NBPG)&0x003ff000)>>12)*sizeof(pt_t);
		if(pt_tmp->pt_pres == 1 && pt_tmp->pt_acc == 1){
			fr_tmp->fr_loadtime = timeCount;
			pt_tmp->pt_acc = 0;
		}
	}
  }
  //kprintf("pt : %x\n", pt);
  pt->pt_base = FRAME0 + source;
  source = (unsigned int)pt/NBPG - FRAME0;
  frm_tab[source].fr_refcnt++;
//  setpdbr(currpid);
  restore(ps);
  return OK;
}


