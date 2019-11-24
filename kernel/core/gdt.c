#include <pagemem.h>
#include <debug.h>
#include <cr.h>
#include <gdt.h>
#include <map_of_mem.h>

seg_desc_t GDT[LENGTH_GDT];

/*
 * Init GDT with first descriptor of segment NULL
 */
void init_gdt(){
  gdt_reg_t gdtr;
  gdtr.desc = GDT;
  gdtr.limit = sizeof(GDT)-1;
  memset((void *)&GDT[0],0,sizeof(seg_desc_t)); //Set first segment at 0
  set_gdtr(gdtr);/* Load GDT */
}
/*
 * Add segment descriptor to GDT
 */
void add_desc_gdt(int index, uint32_t base, uint32_t limit, unsigned short type, unsigned short privilege){
  gdt_reg_t gdtr;get_gdtr(gdtr);  /* Load GDT */
  gdtr.desc[index].base_1 = base;
  gdtr.desc[index].base_2 = base >> 16;
  gdtr.desc[index].base_3 = base >> 24;
  gdtr.desc[index].limit_1 = limit;
  gdtr.desc[index].limit_2 = limit >> 16;
  gdtr.desc[index].type =  type;
  if(type==SEG_DESC_SYS_TSS_AVL_32 || type==SEG_DESC_SYS_TSS_BUSY_32)
    gdtr.desc[index].s = 0; /* Descriptor of TSS segment */
  else 
    gdtr.desc[index].s = 1; /* Descriptor of code or data segment */
  gdtr.desc[index].dpl = privilege;
  gdtr.desc[index].p = 1; 
  gdtr.desc[index].d = 1; /* 32bit */
  gdtr.desc[index].g = 1; /* limit explain in page size (4Ko) */
}

/*
 * Create PGD at 0x600000
 */
void init_pgd(void){
  debug("[%s] Success ! \n\n",__func__);
  int i;
  pde32_t * pgd = (pde32_t *) ADDR_GDT;             /* Create pointer of PGD */
  pde32_t * ptb0 = (pde32_t *)ADDR_PTB_0;           /* Create first kernel PTB_0  [0 - 0x400000] */
  pde32_t * ptb1 = (pde32_t *)ADDR_PTB_1;           /* Create second kernel PTB_1 [0x400000 - 0x800000] */
  pde32_t * ptb_user = (pde32_t *)ADDR_PTB_USER;    /* Create third user PTB_USER   [0x800000 - 0xC00000] */

  set_cr3((uint32_t)pgd);   /* load @PDG in CR3 */ 
  
  memset((void *) pgd, 0,PAGE_SIZE);      //Clean PGD 
  memset((void *) ptb0, 0,PAGE_SIZE);     //Clean PTB_0 
  memset((void *) ptb1, 0,PAGE_SIZE);     //Clean PTB_1 
  memset((void *) ptb_user, 0, PAGE_SIZE);//Clean PTB_USER

  /* A savoir lors de l'activation de la pagination les adresses mappées sont additionées multiplié par 0x1000 sur PAGE_SIZE */

  //Mappinge of PTB_0 [0x000000 - 0x400000]
  for(i=0;i<1024;i++){  
    pg_set_entry(&ptb0[i],PG_KRN|PG_RW,i);
  }

  //Mappinge of PTB_1 [0x400001 - 0x800000]
  for(i=0;i<1024;i++){
    pg_set_entry(&ptb1[i],PG_KRN|PG_RW,i+0x400);
  }

  /* Add PTB_{0,1} to PGD */
  pg_set_entry(&pgd[0],PG_KRN|PG_RW, page_nr(&ptb0[0]));      /* PDE - [0x000000 - 0x400000] */
  pg_set_entry(&pgd[1],PG_KRN|PG_RW, page_nr(&ptb1[0]));      /* PDE - [0x400001 - 0x800000] */
  

 
  pte32_t * pte_task_data_user = (pte32_t *)ADDR_TASK_USER1_DATA;
  pte32_t * pte_task_code_user = (pte32_t *)ADDR_TASK_USER1_CODE;
  pte32_t * pte_task_stack_user = (pte32_t *)ADDR_TASK_USER1_STACK_USER;
  pte32_t * pte_task_stack_kernel_user = (pte32_t *)ADDR_TASK_USER1_STACK_KERNEL;
  
  //Mapping of share data user1/user2
  pg_set_entry(&ptb_user[0], PG_USR | PG_RW, page_nr(pte_task_data_user));
  
  //Mapping of code user1
  pg_set_entry(&ptb_user[1], PG_USR | PG_RO, page_nr(pte_task_code_user));

  //Mapping of stack user1 
  pg_set_entry(&ptb_user[2], PG_USR | PG_RW, page_nr(pte_task_stack_user));

  //Mapping of stack kernel user1
  pg_set_entry(&ptb_user[3], PG_KRN | PG_RW, page_nr(pte_task_stack_kernel_user));
  
 
  pte_task_data_user = (pte32_t *)ADDR_TASK_USER2_DATA;
  pte_task_code_user = (pte32_t *)ADDR_TASK_USER2_CODE;
  pte_task_stack_user = (pte32_t *)ADDR_TASK_USER2_STACK_USER;
  pte_task_stack_kernel_user = (pte32_t *)ADDR_TASK_USER2_STACK_KERNEL;
    
  //Mapping of code user2
  pg_set_entry(&ptb_user[5], PG_USR | PG_RO, page_nr(pte_task_code_user));

  //Mapping of stack user2 
  pg_set_entry(&ptb_user[6], PG_USR | PG_RW, page_nr(pte_task_stack_user));

  //Mapping of stack kernel user2
  pg_set_entry(&ptb_user[7], PG_KRN | PG_RW, page_nr(pte_task_stack_kernel_user));

  /* Add PTB_USER to PGD */
  pg_set_entry(&pgd[2],PG_KRN|PG_RW, page_nr(&ptb_user[0]));  /* PDE - [0x800001 - 0x809000] */
}   
