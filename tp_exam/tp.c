/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <segmem.h>
#include <pagemem.h>
#include <string.h>
#include <cr.h>
#include <asm.h>
#include <intr.h>

/* My files */
#include <task.h>
#include <gdt.h>
#include <map_of_mem.h>
#include <display.h>

extern info_t *info;

//Mode de priviliège
#define RING_3 3
#define RING_0 0 

/*
 * Chargement des pointeurs de segment
 * Pour rappel, étant en ring 0 : 
 *    - On peut modifier de la data  dans un segment data en RING_0 ou RING_3
 *    - On peut modifier du code dans un segment RING_0 (uniquement)
 */
void load_register(int priv_level){
  debug("[%s] : ",__func__);
  if(priv_level==RING_0){
    debug("Chargement des pointeurs de segment RING %d\n",priv_level); 
    set_cs(c0_sel); 
    set_ss(d0_sel);
    set_ds(d0_sel); set_es(d0_sel); set_fs(d0_sel); set_gs(d0_sel);
  } else {
    debug("Chargement des pointeurs de segment RING %d\n",priv_level);     
    set_ds(d3_sel); set_es(d3_sel); set_fs(d3_sel); set_gs(d3_sel);
  }
}

/*
 * Initialise le descripteur de la TSS  
 */
void init_tss(){
  //Création dans la gdt du descripteur 
  add_desc_gdt(ts_idx,0,sizeof(tss_t),SEG_DESC_SYS_TSS_AVL_32,0);
  set_tr(ts_sel); //Chargement de la TSS dans le registre tr
  display_gdt();
} 

/*
 * Gestion du syscall avec des arguments
 */
void syscall_isr(){
   asm volatile (
      "leave ; pusha        \n"
      "mov %esp, %eax       \n"
      "call syscall_handler \n"
      "popa ; iret"
      );
}
/*
 * Hnadler de l'interruption
 */
void __regparm__(1) syscall_handler(int_ctx_t *ctx){
    debug("print syscall: %s", ctx->gpr.esi);
}
/*
 * Création de l'interruption permettant un affichage même en ring3
 */
void debug_int48(){
  int_desc_t *dsc;
  idt_reg_t idtr;
  get_idtr(idtr);
  dsc = &idtr.desc[48]; //Ajout du descripteur d'int dans idt 48
  
  dsc->dpl =3; //Appelable en ring 3  
  //Mise en place du handler
  dsc->offset_1 = (uint16_t)((uint32_t)syscall_isr);
  dsc->offset_2 = (uint16_t)(((uint32_t)syscall_isr)>>16);
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

void launch_task(task_t *task){
 tss_change_s0_esp(task->kernel_stack);
 asm volatile (
      "mov %0,%%esp  \n" // On met à jour la stack kernel user1
      
      "push %1 \n" //On push ss
      "push %2 \n" //On push esp
      "push %3 \n" //On push EFLAGS
      "push %4 \n" //On push CS
      "push %5 \n" //On push EIP
      "popa \n"
      "iret"
      ::
       "r"(task->kernel_stack),
       "r"(task->ss_task),
       "r"(task->user_stack),
       "r"(task->flags_task),
       "r"(task->cs_task),
       "r"(task->addr_task_code)
      );
}


/*
 * main
 */
void tp(){
 
  force_interrupts_on(); 
  /* CRÉATION DE LA GDT */
  explain_desc_gdt();                               //Affichage sympatoch pour comprendre la GDT
  init_gdt();                                       //Initialisation de la GDT avec le segment 0 - NULL
  add_desc_gdt(c0_idx,0,0xfffff,SEG_DESC_CODE_XR,RING_0);//Création du segment RING 0 - CODE de 0 à 4G
  add_desc_gdt(d0_idx,0,0xfffff,SEG_DESC_DATA_RW,RING_0);//Création du segment RING 0 - DATA de 0 à 4G
  add_desc_gdt(c3_idx,0,0xfffff,SEG_DESC_CODE_XR,RING_3);//Création du segment RING 3 - CODE de 0 à 4G
  add_desc_gdt(d3_idx,0,0xfffff,SEG_DESC_DATA_RW,RING_3);//Création du segment RING 3 - CODE de 0 à 4G
  //display_gdt();                                    //Affichage de la GDT
  /*****************/  

  /* CRÉATION DE LA TABLE TSS */
  /*Le descripteur de la table TSS (faisant le lien entre la pile R0 et R3) 
    Et ayant la même structure qu'un descripteur de segment, on la met à la suite de la GDT pour simplifier même si elle ne lui appartient pas cf(p.336)*/
  explain_desc_tss();
  init_tss();
  display_tss();
  /*****************/
  
  /* Mise à jour des pointeurs de segment DS/ES/FS/GS pour le lancement d'un tâche RING_3 */
  load_register(RING_3);
  /*****************/

  /* Activation de la pagination */
  init_pgd();
  enable_paging();
  display_pgd();
  /*****************/
  
  /* Ajout du code user dans la segment de code user */
  task_t task1;
  task_t task2;

  init_user_task(&task1,&user1,ADDR_TASK_USER1_DATA,ADDR_TASK_USER1_CODE,ADDR_TASK_USER1_STACK_USER,ADDR_TASK_USER1_STACK_KERNEL); 
  init_user_task(&task2,&user2,ADDR_TASK_USER2_DATA,ADDR_TASK_USER2_CODE,ADDR_TASK_USER2_STACK_USER,ADDR_TASK_USER2_STACK_KERNEL); 
  /*****************/
  /* Lancement d'un tâche utilisateur user1 */
  launch_task(&task1);

}
