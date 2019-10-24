/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <segmem.h>
#include <asm.h>
extern info_t *info;

/*
 * Active l'interruption (aucune interruption ne correspond donc déclenche la 32 (1ère utilisateur)) 
 */
void interrupt(){
  //CLI : Clear Interrup Glaf
  //STI : Set Interrupt Flag
  while(1)
    asm volatile("sti");
}
/*
 * Handler maison de #BP (breakpoint) : interruption numéro 3
 */
void bp_handler(){
  debug("[%s] @handler = %p \n",__func__,bp_handler);
  uint32_t eip;
  asm volatile ("mov 4(%%ebp), %0": "=r"(eip)); //On recupère la base de la pile et on décale de 4 pour avoir le sommet
  debug("[%s] @eip = %p\n",__func__,eip);
  
  //Puisque c'est un handler il faut un retour iret comme pour la fin du interruption
  asm volatile("leave");
  asm volatile("iret"); 
}
/*
 * Déclanche le BP
 */
void bp_trigger(){
  debug("[%s]\n",__func__);
  asm volatile("int3");
  debug("[%s] Fin de l'interruption 3\n",__func__);
}



/***** MAIN ******/
void tp(){
  idt_reg_t idtr;
  get_idtr(idtr);//Init dans intr.c

  debug("\n[%s] @IDT = %p - Length = %d \n",__func__,idtr.addr,(idtr.limit-1)/sizeof(int_desc_t));
  
  //On modifie le descriteur d'interruption uméro 3 pour faire pointé l'interruption sur notre fonction
  int_desc_t * desc_bp = &idtr.desc[3];
  desc_bp->offset_1 = (uint32_t)bp_handler & 0xFFFF;
  desc_bp->offset_2 = (((uint32_t)bp_handler)>>16) & 0xFFFF;
  bp_trigger();
}
