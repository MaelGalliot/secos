/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <segmem.h>
#include <pagemem.h>
#include <string.h>
#include <cr.h>
#include <asm.h>
#include <intr.h>

extern info_t *info;

//Mode de priviliège
#define RING_3 3
#define RING_0 0 

//Selecteur de segment optimisé 
#define c0_idx  1
#define d0_idx  2
#define c3_idx  3
#define d3_idx  4
#define ts_idx  5

#define c0_sel  gdt_krn_seg_sel(c0_idx)
#define d0_sel  gdt_krn_seg_sel(d0_idx)
#define c3_sel  gdt_usr_seg_sel(c3_idx)
#define d3_sel  gdt_usr_seg_sel(d3_idx)
#define ts_sel  gdt_krn_seg_sel(ts_idx)

//Affichage
#define  base(_dsc)  _dsc.base_3<<24 | _dsc.base_2<<16 | _dsc.base_1
#define  limit(_dsc) _dsc.limit_2<<16| _dsc.limit_1

//Création du tableau de descritpeur de GDT
#define LENGTH_GDT 6
seg_desc_t GDT[LENGTH_GDT];

//Addresse des tâches user1
#define ADDR_TASK_USER1 0x801000
#define ADDR_TASK_USER1_DATA 0x801000
#define ADDR_TASK_USER1_CODE 0x802000
#define ADDR_TASK_USER1_STACK_USER 0x803000
#define ADDR_TASK_USER1_STACK_KERNEL 0x804000


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
void tss_change_s0_esp(uint32_t task_esp){
  tss_t tss;
  get_tr(tss);
  //Création de la première tâche de la TSS en ajoutant les champ statique utile
  tss.s0.esp = task_esp; //On met à jour ebp
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
void user1(){
  //int * counter=(int *)ADDR_TASK_USER1_DATA;
  while(1){
    //*counter+=1;
    //asm volatile("int $48"::"S"("test\n")); 
  };
  return;
} 

/*
 * Activation de la pagination, en ajoutant dans CR0 le flag de PG à 1
 */
void enable_pagination(){
  debug("[%s]",__func__);
  uint32_t cr0 = get_cr0(); //Récupération du registre CR0
  cr0 |= CR0_PG; //On met à 1 les flags CR0.PG
  set_cr0(cr0); //On charge CR0
  debug("Activation de la pagination -  CR0 = 0x%x\n",cr0);
}

/*
 * Affichage de la pgd
 */
void display_pgd(){
  debug("[%s]",__func__);
  int i,pde_p=0;
  cr3_reg_t cr3 = {.raw = get_cr3()}; //On récupère CR3 pour connaître l'adresse de la pgd
  pde32_t * pgd = (pde32_t *)0x600000;
  pde32_t * ptb = (pde32_t *)(pgd+PAGE_SIZE);
  debug("Adresse de la PGD chargé dans le noyau (cr3.addr) : %p\n",cr3.addr<<12);
  debug("\n|------------------ PGD -------------------| <-%p\n",pgd);
  for(i=0;i<1024;i++){
    if(pgd[i].p){ //On cherche les PDEs présentes
      pde_p++;
      ptb = (pde32_t *)(pgd+(PAGE_SIZE*(i+1)));
      debug("| PDE:%d, Mappe les @: %p à %p | <- %p\n",i,(ptb[0].addr<<12),(ptb[0].addr<<12)+PAGE_SIZE*1024,&pgd[i]);
    }
  }
  debug("|----------------------------------------|\n"); 
  debug("Nombre de PDE détéctée(s) : %d\n",pde_p);
}

/*
 * Création de la PGD (adresse abritraire de la PGD à 0x600000)
 */
void init_pgd(){
  debug("[%s]\n",__func__);
  int i;
  pde32_t * pgd = (pde32_t *)0x600000; //Création du pointeur de la PGD
  pde32_t * ptb0 = (pde32_t *)(pgd+PAGE_SIZE);  //Création de la première entrée de la PGD pointant sur la PTB
  pde32_t * ptb1 = (pde32_t *)(pgd+PAGE_SIZE*2);
   
  set_cr3((uint32_t)pgd); //Chargement de la PGD dans CR3
  
  memset((void *) pgd, 0,PAGE_SIZE);//Clean de la PGD soit 4096oct
  memset((void *) ptb0, 0,PAGE_SIZE);//Clean de la PTB 
  memset((void *) ptb1, 0,PAGE_SIZE);//Clean de la PTB 
  //for(i=0;i<1024;i++)
  //  debug("&pgd[%d]=%p, ptb[%d].p = %d\n",i,&pgd[i],i,pgd[i].p); 

  /* A savoir que lors de l'activation de la pagination les adresses mappés dans cette fonction sont additionées à CR3 et multiplié par 32 (4 oct) */
  for(i=0;i<1024;i++){  //Mappage de la PGD, qui est mappé par la première entrée de la PTB
    pg_set_entry(&ptb0[i],PG_KRN|PG_RW,i);//PTB[0] mappe les adresse de [0x000000 - 0x400000] en PG_KRN et PG_RW)
    //debug("pgd[0].ptb[%d].addr = %p (%p),  ptb[%d].p = %d\n",i,ptb[i].addr,ptb[i].addr<<12,i,ptb[i].p); 
  }

  pg_set_entry(&pgd[0],PG_KRN|PG_RW, page_nr(&ptb0[0])); //PDE mappe de [0x000000 - 0x400000]

  for(i=0;i<1024;i++){
    pg_set_entry(&ptb1[i],PG_KRN|PG_RW,i+0x400); //PTB[1] mappe la PTB de [0x400001 - 0x800000] (0x1000 * 0x4000)
    //debug("pgd[1].ptb[%d].addr = %p (%p),  ptb[%d].p = %d\n",i,ptb[i].addr,ptb[i].addr<<12,i,ptb[i].p); 
  }

  pg_set_entry(&pgd[1],PG_KRN|PG_RW, page_nr(&ptb1[0])); //PDE mappe de [0x400001 - 0x800000]


  //Init de la page USER1
  pde32_t * ptb_user1 = (pde32_t *)(pgd+PAGE_SIZE*3);
  memset((void *) ptb_user1, 0, PAGE_SIZE);//Clean de la ptb
 
  pg_set_entry(&pgd[2],PG_KRN|PG_RW, page_nr(&ptb_user1[0])); //PDE mappe de [0x400001 - 0x800000]

 
  pte32_t * pte_task_data = (pte32_t *)ADDR_TASK_USER1_DATA;
  pte32_t * pte_task_code = (pte32_t *)ADDR_TASK_USER1_CODE;
  pte32_t * pte_task_stack_user = (pte32_t *)ADDR_TASK_USER1_STACK_USER;
  pte32_t * pte_task_stack_kernel = (pte32_t *)ADDR_TASK_USER1_STACK_KERNEL;
  
  //Mappage de la section data de user1
  pg_set_entry(&ptb_user1[0], PG_USR | PG_RW, page_nr(pte_task_data));
  
  //Mappage de la section code de user1
  pg_set_entry(&ptb_user1[1], PG_USR | PG_RO, page_nr(pte_task_code));

  //Mappage de la stack user 
  pg_set_entry(&ptb_user1[2], PG_USR | PG_RW, page_nr(pte_task_stack_user));

  //Mappage de la stack kernel 
  pg_set_entry(&ptb_user1[3], PG_KRN | PG_RW, page_nr(pte_task_stack_kernel));
  
}   

typedef struct task_t task_t;
struct task_t {
  uint32_t user_stack;
  uint32_t kernel_stack;
  uint32_t addr_task_code;
  uint32_t addr_task_data;
  uint32_t cs_task;
  uint32_t ss_task;
  uint32_t flags_task;
};
/*
 * Initilisation de la tâche user en ajoutant la tâche dans une page USER, et en ajoutant les entrées de la TSS 
 */
void init_task_user1(task_t * task){
  debug("[%s]\n",__func__);
  memset(task, 0, sizeof(task)); //On clean la zone de la tâche
  memcpy((char *) ADDR_TASK_USER1_CODE, &user1,0x1000);//On copie le code dans la tâche dans l'adresse prévue 
  //Les pages de la tâche USER1 sont mappées en statique dans la fonction 
  task->addr_task_data = ADDR_TASK_USER1_DATA;
  task->addr_task_code = ADDR_TASK_USER1_CODE;
  task->user_stack = ADDR_TASK_USER1_STACK_USER+0x1000;//La pile commence à l'envers
  task->kernel_stack = ADDR_TASK_USER1_STACK_KERNEL+0x1000; 
  task->cs_task = c3_sel;
  task->ss_task = d3_sel;
  task->flags_task= get_flags();

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
  enable_pagination();
  display_pgd();
  /*****************/
  
  /* Ajout du code user dans la segment de code user */
  task_t task1;
  
  init_task_user1(&task1); 
   
  /*****************/
  


  /* Lancement d'un tâche utilisateur user1 */
  tss_change_s0_esp(task1.kernel_stack);
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
       "r"(task1.kernel_stack),
       "r"(task1.ss_task),
       "r"(task1.user_stack),
       "r"(task1.flags_task),
       "r"(task1.cs_task),
       "r"(task1.addr_task_code)
      );
}
