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

void display_fullptb(pde32_t * addr_pde){
  debug("[%s]",__func__);
 // int i;
  debug("Adresse de la PTB : %p\n",addr_pde->addr);
/*  for(i=0;i<1024;i++)
    debug("Contenu PTB[%d](%p) = %p - Pointe sur la page mappé:%p en RW:%d et P:%d \n",i,&addr_ptb[i],addr_ptb[i].raw, addr_ptb[i].addr ,addr_ptb[i].rw, addr_ptb[i].p);*/
}
void display_briefpgd(pde32_t * addr_pde){
  debug("[%s]\n",__func__);
  int i,nbr_pde=0;
   for(i=0;i<1024;i++){ //Affichage de toute les PDE
    if(addr_pde->p==1){
      nbr_pde++;
      debug("%d - %p présent\n",i,addr_pde[i]);  
    }
  }
  debug("Adresse de la PGD : %p, contenant %d ptb présente \n",addr_pde,nbr_pde);
  debug("Adresse de la PTB : %p\n",addr_pde->addr);
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
    pg_set_entry(&ptb[i],PG_RW|PG_KRN,i); //Mappage de la page PGD  
  memset((void*)pgd,0,1024); //On clean la pgd
  pg_set_entry(&pgd[0],PG_KRN|PG_RW,page_nr(ptb)); //Créer une page à l'index 0 de la PGD avec les paramètres et on aura la ptb qu'on vient de créer 

  // 6: il faut mapper les PTBs également
  pte32_t *ptb2 = (pte32_t*)0x602000;
  for(i=0;i<1024;i++)
     pg_set_entry(&ptb2[i], PG_KRN|PG_RW, i+0x1000); //Mappage de la PTB
  pg_set_entry(&pgd[1], PG_KRN|PG_RW, page_nr(ptb2));

  display_briefpgd(pgd);

  activatePagination(); //Lors de l'activation de la pagination les adresses des pages pointées par les PTE sont multiplié par 32 (car les PTE font 4 oct) et additionnée à 0x600000 
 
  display_briefpgd(pgd);
}
