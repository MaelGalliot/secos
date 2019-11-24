#include <gdt.h>
#include <debug.h>
#include <cr.h>
#include <pagemem.h>
#include <map_of_mem.h>

/*
 * Display of current PGD in kernel
 */
void display_pgd(){
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
 * Affichage de la GDT qui est chargé dans le noyau
 */
void display_gdt(){
  gdt_reg_t gdtr; //Création du pointeur vers la gdt
  unsigned int i; //int = long = 4 octets 
  get_gdtr(gdtr);//Récupère la gdt actuel avec instruction asm sgdt
  unsigned int gdt_length = (gdtr.limit+1)/sizeof(seg_desc_t);//Taille de la gdt

  debug("\n[%s] @GDT = %p - Length = %d \n",__func__,gdtr.desc,gdt_length);
  debug("|----------------------------------- GDT -----------------------------------|\n");
  for(i=0; i<gdt_length;i++){ //On boucle pour tous les descripteurs
           debug("|-Num : %d --- Base : %p --- Limit : 0x%x ---P : 0x%x ---g : 0x%x ----------| <-%p\n",i,base(gdtr.desc[i]),limit(gdtr.desc[i]),gdtr.desc[i].p,gdtr.desc[i].g,&(gdtr.desc[i]));
    debug("|-Type : %d --- s : 0x%x --- dpl : 0x%x --- avl : 0x%x --- d/b : 0x%x -----------|\n",gdtr.desc[i].type,gdtr.desc[i].s,gdtr.desc[i].dpl,gdtr.desc[i].avl,gdtr.desc[i].d);
    debug("|---------------------------------------------------------------------------|\n"); 
  }
}

/*
 * Explication de chaque champ de la GDT
 */
void explain_desc_gdt(){
  debug("\n[%s] EXPLICATION DES DESCRIPTEURS DE SEGMENT \n",__func__);
  debug("\tNum : Numéro du descripteur\n\tBase : @linéaire sur 32 bits, où débute le segment en mémoire par rapport à la mémoire disponible \n\tLimit : 20  bit, sur la longueur du segment (max 4G si à 0xfffff)\n\tP(Present) : 1 bit, détermine si le segment est présent/chargé/référencé dans la mémoire\n\tDPL(Descriptor Priviliege-Level : 2 bit,  indique le niveau de privilège du segment mémoire noyau ring 0 -> 0\n\tAVL(Available To Software) : 1 bit, dispo pour le logiciel (le proc ne s'en occupe pas)\n\td/b(Default Operand Size) : 1 bit, taillle des instructions et des données manipulées. 1= 32bits 0=16bits(défaut)\n\tG(Granularity) : 1 bit, 0 si la limit est exprimé en octet, 1 en page (de 4Ko)\n\tS : 1 bit, à 0 pour les descripteur de segment (code/data) et à 1 pour les descripteur système(TSS/LDT/Gate)\n\tType : 2 bits, en fonction de S change le type de segment pointé");
  debug("\nPour plus d'info télécharger AMD64 Architecture Prgrammer's Manual Volume 2 :System Programming (cf p80)\n");
}

/*
 * Explication des champs de la TSS
 */
void explain_desc_tss(){
  debug("\n[%s] EXPLICATION DU DESCRIPTEURS DE TSS \n\n",__func__);
  debug("Champ statique (information lu lors du switch de processus): \n\t Stack pointeur : Contient les stack pointeur de la tâche pour les priv 0/1/2\n\t CR3 : Contient entre autre l'@ de la PGD\n\t LDT : Contient le selecteurde la LDT de la tâche\n\t Bitmap I/O permission : cf 337 pas traité ici mais permet de spécifié les ports accessible par la tâche\n\n");
  debug("Champ dynamique (lu et modifié lors du witch de processus) : \n\t Link : Contient le selecteur de TSS de la tâche précedente\n\t EIP : L'@ de la prochaine instruction à exectuer quand la tâce est restorée \n\t EFLAGS : COntient une image des flags lorsque la tâce à été suspendu (d'où la mise en place d'un contexte lors d'un changement de tâche (tp3)\n\t GeneralRegister : Contient les copies de EAX, EBX, ECX, EDX, ESP, EBP, ESI, et EDI lorsque la tâche a été suspendue\n\t SegmentRegister : COntient la copie de ES, CS, SS, DS, FS lorsque la tâche à été suspendue\n\n");
  debug("\nPour plus d'info télécharger AMD64 Architecture Prgrammer's Manual Volume 2 :System Programming (cf p337)\n"); 
}

/*
 * Affichage de la TSS de l'OS
 */
void display_tss(){
  debug("\n[%s]",__func__);
  gdt_reg_t gdtr;
  tss_t tss;
  get_tr(tss);
  get_gdtr(gdtr);//Chargement de la gdt
  debug("Adresse de la TSS [%p-%p] - taille de 0x%x\n",(&tss)+(base(gdtr.desc[ts_idx])),(&tss)+((limit(gdtr.desc[ts_idx]))-(base(gdtr.desc[ts_idx]))),(limit(gdtr.desc[ts_idx]))-(base(gdtr.desc[ts_idx])));
}