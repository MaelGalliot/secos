#ifndef TASK_H
#define TASK_H
      
#include <types.h>

/* Addres of task user in memory */
#define ADDR_TASK_USER1                 0x801000 /*   TASK 1   */
#define ADDR_TASK_USER1_DATA            0x801000 /* |  DATA  | <-- 0x801000 */
#define ADDR_TASK_USER1_CODE            0x802000 /* |  CODE  | <-- 0x802000 */
#define ADDR_TASK_USER1_STACK_USER      0x803000 /* | USTACK | <-- 0x803000 */
#define ADDR_TASK_USER1_STACK_KERNEL    0x804000 /* | KSTACK | <-- 0x804000 */

/* Addres of task user in memory */
#define ADDR_TASK_USER2                 0x806000 /*   TASK 2   */
#define ADDR_TASK_USER2_DATA            0x801000 /* |  DATA  | <-- 0x806000 */
#define ADDR_TASK_USER2_CODE            0x807000 /* |  CODE  | <-- 0x807000 */
#define ADDR_TASK_USER2_STACK_USER      0x808000 /* | USTACK | <-- 0x808000 */
#define ADDR_TASK_USER2_STACK_KERNEL    0x809000 /* | KSTACK | <-- 0x809000 */


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


char * init_user_task(task_t * task, void * user_code, uint32_t addr_data, uint32_t addr_code, uint32_t addr_stack_user, uint32_t addr_kernel_stack);
void tss_change_s0_esp(uint32_t esp_of_current_task);
void user1(void);
void user2(void);

#endif