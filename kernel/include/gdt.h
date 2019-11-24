#ifndef PAGING_H
#define PAGING_H

#include <segmem.h>

//Display
#define  base(_dsc)  _dsc.base_3<<24 | _dsc.base_2<<16 | _dsc.base_1
#define  limit(_dsc) _dsc.limit_2<<16| _dsc.limit_1

//Create Array for GDT descriptor
#define LENGTH_GDT 6

/* Functions of paging.c */
void init_pgd(void);
void init_gdt(void);
void add_desc_gdt(int index, uint32_t base, uint32_t limit, unsigned short type, unsigned short privilege);

#endif