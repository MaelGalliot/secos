/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <segmem.h>
#include <pagemem.h>
#include <string.h>
#include <cr.h>
#include <asm.h>
#include <intr.h>
#include <map_of_mem.h>
/*
** My files : task.c | task.h, paging.c | paging.h
 */
#include <task.h>
#include <paging.h>

extern info_t *info;

//Mode de priviliège
#define RING_3 3
#define RING_0 0 

//Affichage
#define  base(_dsc)  _dsc.base_3<<24 | _dsc.base_2<<16 | _dsc.base_1
#define  limit(_dsc) _dsc.limit_2<<16| _dsc.limit_1

//Création du tableau de descritpeur de GDT
#define LENGTH_GDT 6
seg_desc_t GDT[LENGTH_GDT];


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
 * Initialise la GDT avec un le premier descritpeur de segment NULL
 */
void init_gdt(){
  gdt_reg_t gdtr;   //Création du pointeur de GDT
  gdtr.desc = GDT;  //Application de ce pointeur
  gdtr.limit = sizeof(GDT)-1; //Appliation de la limite de la GDT
  memset((void *)&GDT[0],0,sizeof(seg_desc_t)); //Init du segment 0 à null
  set_gdtr(gdtr);//Chargement de la gdt
}
/*
 * Ajout un descripteur de segment
 */
void add_desc_gdt(int index, uint32_t base, uint32_t limit, unsigned short type, unsigned short privilege){
  gdt_reg_t gdtr;
  get_gdtr(gdtr);//Chargement de la gdt
  gdtr.desc[index].base_1 = base;
  gdtr.desc[index].base_2 = base >> 16;
  gdtr.desc[index].base_3 = base >> 24;
  gdtr.desc[index].limit_1 = limit;
  gdtr.desc[index].limit_2 = limit >> 16;
  gdtr.desc[index].type =  type;
  if(type==SEG_DESC_SYS_TSS_AVL_32 || type==SEG_DESC_SYS_TSS_BUSY_32){
    gdtr.desc[index].s = 0; //Segment TSS
    
  }
  else 
    gdtr.desc[index].s = 1; //Segment de code ou de data
  gdtr.desc[index].dpl = privilege;
  gdtr.desc[index].p = 1; //Le descripteur est présent
  gdtr.desc[index].d = 1; //32bit 
  gdtr.desc[index].g = 1; //La limit est exprimee en page de 4Ko
}

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
  tss_change_s0_esp(task1.kernel_stack);
 /* asm volatile (
      "mov %0,%%esp  \n" // On met à jour la stack kernel user1
      
      "push %1 \n" //On push ss
      "push %2 \n" //On push esp
      "push %3 \n" //On push EFLAGS
      "push %4 \n" //On push CS
      "push %5 \n" //On push EIP
      "popa \n"
      "iret"
      ::
       "r"(task1.kernel_stack),
       "r"(task1.ss_task),
       "r"(task1.user_stack),
       "r"(task1.flags_task),
       "r"(task1.cs_task),
       "r"(task1.addr_task_code)
      );*/
}
