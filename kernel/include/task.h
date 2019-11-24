#ifndef TASK_H
#define TASK_H
      
#include <types.h>
#include <map_of_mem.h>

/* Struct of task */
typedef struct task_t {
  uint32_t user_stack;
  uint32_t kernel_stack;
  uint32_t addr_task_code;
  uint32_t addr_task_data;
  uint32_t cs_task;
  uint32_t ss_task;
  uint32_t flags_task;
} __attribute__((packed)) task_t;

/* Functions of task.c */
void init_user_task(task_t * task, void * user_code, uint32_t addr_data, uint32_t addr_code, uint32_t addr_stack_user, uint32_t addr_kernel_stack);
void tss_change_s0_esp(uint32_t esp_of_current_task);
void user1(void);
void user2(void);

#endif