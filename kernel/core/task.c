#include <task.h>
#include <string.h>
#include <debug.h>
#include <segmem.h>
#include <pagemem.h>

/*
 * Init a user task in memory  
 * ------------------------
 * Example in memory : 
 *   TASK 1   
 * |  DATA  | <-- 0x806000 
 * |  CODE  | <-- 0x807000 
 * | USTACK | <-- 0x808000 
 * | KSTACK | <-- 0x809000 
 */
void init_user_task(task_t * task, void * user_code, uint32_t addr_data, uint32_t addr_code, uint32_t addr_stack_user, uint32_t addr_kernel_stack){
  debug("[%s] to [%p - %p] \n",__func__,addr_data,addr_kernel_stack);

  memcpy((char *) addr_code, &user_code, PAGE_SIZE);    /* Copy the user code to section code of task */

  task->addr_task_data  = addr_data;
  task->addr_task_code  = addr_code;
  task->user_stack      = addr_stack_user + PAGE_SIZE;  /* Stack starts at the end */
  task->kernel_stack    = addr_kernel_stack + PAGE_SIZE;/* Stack starts at the end */
  task->cs_task         = c3_sel;
  task->ss_task         = d3_sel;
  task->flags_task      = get_flags();
}

/*
 * Update ebp of the task currently execute 
 */
void tss_change_s0_esp(uint32_t esp_of_current_task){
  tss_t tss; get_tr(tss);
  tss.s0.esp = esp_of_current_task; /* Update ebp */
}

/*
 * First user task who counts
 */
void user1(){
  //int * counter=(int *)ADDR_TASK_USER1_DATA;
  while(1){
    //*counter+=1;
    //asm volatile("int $48"::"S"("test\n")); 
  };
} 

/*
 * Second user task who display counter
 */
void user2(){
  while(1){
    //asm volatile("int $48"::"S"("test\n")); 
  };
} 