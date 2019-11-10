/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>

extern info_t *info;


/*
 * Tâche de l'utilsiateur 1 
 */ 
void user1(void){
  debug("[%s",__func__);
  while(1);
}

/* 
 * Tâche de l'utilisateur 2
 */
void user2(void){
  debug("[%s",__func__);
  while(1);
}



void tp()
{
}
