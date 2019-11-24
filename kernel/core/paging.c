#include <pagemem.h>
#include <debug.h>
#include <cr.h>
#include <paging.h>
#include <map_of_mem.h>

/*
 * Display of current PGD in kernel
 */
void display_pgd(void){
  debug("[%s] ",__func__);
  int i;
  cr3_reg_t cr3 = {.raw = get_cr3()}; /* CR3 contains address of PGD*/
  pde32_t * pgd = (pde32_t *) ADDR_GDT;
  pde32_t * ptb = (pde32_t *)(pgd+PAGE_SIZE);
  debug("Adress of the PGD load in kernel (cr3.addr) : %p",cr3.addr<<12);
  debug("\n|------------------ PGD -------------------| <-%p\n",pgd);
  for(i=0;i<1024;i++){
    if(pgd[i].p){ //On cherche les PDEs présentes
      ptb = (pde32_t *)(pgd+(PAGE_SIZE*(i+1)));
      debug("| PDE:%d, Mapping addr: %p à %p | <- %p\n",i,(ptb[0].addr<<12),(ptb[0].addr<<12)+PAGE_SIZE*1024,&pgd[i]);
    }
  }
  debug("|------------------------------------------|\n\n"); 
}

/*
 * Enable paging, CR0.PG = 1
 */
void enable_paging(void){
  debug("[%s]",__func__);
  uint32_t cr0 = get_cr0(); /* Get CR0 */
  set_cr0(cr0 |= CR0_PG);   /* Load CR0 with CR0.PG = 1*/
  debug(" Enable of paging -  CR0 = 0x%x\n\n",cr0);
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
  
  //Mapping of data user1
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
  
  //Mapping of data user2
  pg_set_entry(&ptb_user[4], PG_USR | PG_RW, page_nr(pte_task_data_user));
  
  //Mapping of code user2
  pg_set_entry(&ptb_user[5], PG_USR | PG_RO, page_nr(pte_task_code_user));

  //Mapping of stack user2 
  pg_set_entry(&ptb_user[6], PG_USR | PG_RW, page_nr(pte_task_stack_user));

  //Mapping of stack kernel user2
  pg_set_entry(&ptb_user[7], PG_KRN | PG_RW, page_nr(pte_task_stack_kernel_user));

  /* Add PTB_USER to PGD */
  pg_set_entry(&pgd[2],PG_KRN|PG_RW, page_nr(&ptb_user[0]));  /* PDE - [0x800001 - 0x809000] */
}   
