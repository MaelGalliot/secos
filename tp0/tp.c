/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <grub_mbi.h> 

extern info_t   *info;
extern uint32_t __kernel_start__;
extern uint32_t __kernel_end__;

void tp() {
	

	//Debug	
	debug("kernel mem [0x%x - 0x%x]\n", &__kernel_start__, &__kernel_end__);
   	debug("MBI flags 0x%x\n", info->mbi->flags);
	
	//Initalisation des variables
	multiboot_uint32_t start_mbi; 
	multiboot_uint32_t end_mbi;	
	multiboot_memory_map_t * p_mmap;	
	/* 
	 * La struct info_t contient une struct mbi_t(multiboot_info_t) -- cf info.h
	 * La struct multiboot_info contient les informations du boot -- cf grub_mbi.h 
	 */

	start_mbi = info->mbi->mmap_addr; //Récupération du début de la VM
        end_mbi = info->mbi->mmap_addr + info->mbi->mmap_length + start_mbi; //Récupération de la fin de la VM
	p_mmap = (multiboot_memory_map_t *) start_mbi; //mmap_addr contient l'@ du premier objet de type multiboot_memory_map_t 
	
	//Affichage
	debug("\n\n[TP0] - OS \n\n");
	debug("Cartographie de la VM :  [0x%X - 0x%X], taille : 0x%x\n",start_mbi,end_mbi,info->mbi->mmap_length);
	
	/*
	 *  multiboot_memory_map_t/ struct multiboot_mmap_entry -- cf grub_mbi.h
	 */
	debug("0x%X\n",p_mmap);

	//Boucle d'affichage
	debug("[Adresse] \t| Type | Size | Len\n");	
	while(p_mmap<(multiboot_memory_map_t *)end_mbi){
		debug("[0x%X - 0x%X] | %d | %ld | 0x%X \n",p_mmap->addr+start_mbi,p_mmap->addr+p_mmap->len+start_mbi, p_mmap->type,p_mmap->size,p_mmap->len);
		p_mmap+=sizeof(multiboot_memory_map_t);
	}	
}
