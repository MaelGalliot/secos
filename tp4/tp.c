/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <pagemem.h>
#include <info.h>
#include <cr.h>

extern info_t *info;


void displayCR3(void){
  debug("[%s]",__func__);
  //Création du buffer du registre CR3 qui correspond à l'@ de la PGD
  debug(" CR3 = %p\n",get_cr3());
}

void activatePagination(void){
  debug("[%s]",__func__);
  uint32_t cr0;
  cr0 = get_cr0();
  cr0 |= CR0_PG; 
  set_cr0(cr0);
  debug(" CR0 = 0x%x\n",cr0);
}
void tp(){
  displayCR3();  

  //Création du buffer du registre CR3 contenant l'adresse de la PGD (soit la première entrée de la PDE)
  pde32_t * pgd = (pde32_t *)0x600000;
  pde32_t * ptb = (pde32_t *)0x601000;
  set_cr3((uint32_t) pgd);

  //Allocation de la PGD
  int i;
  for(i=0;i<1024;i++)
    pg_set_entry(&ptb[i],PG_RW|PG_KRN,i); //Création de la page PGD  
  memset((void*)pgd,0,1024); //On clean la pgd
  pg_set_entry(&pgd[0],PG_KRN|PG_RW,page_nr(ptb)); //Créer une page à l'index 0 de la PGD avec les paramètres et on aura la ptb qu'on vient de créer 

  //Allocation de la première PDE
  for(i=0;i<1024;i++)
    pg_set_entry(&ptb[i],PG_RW|PG_KRN,i); //Création de la page PGD
  memset((void*)pgd,0,1024); //On clean la pgd
  pg_set_entry(&pgd[0],PG_KRN|PG_RW,page_nr(ptb));

   
  
  activatePagination();



}
