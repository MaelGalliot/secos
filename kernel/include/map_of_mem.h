#ifndef MAP_OF_MEM_H
#define MAP_OF_MEM_H

/* Address of GDT/PDG in memory */
#define ADDR_GDT                        0x600000
#define ADDR_PTB_0                      ADDR_GDT + PAGE_SIZE
#define ADDR_PTB_1                      ADDR_GDT + 2*PAGE_SIZE
#define ADDR_PTB_USER                   ADDR_GDT + 3*PAGE_SIZE

/* Address of task user in memory */
#define ADDR_TASK_USER1                 0x801000                      /*   TASK 1   */
#define ADDR_TASK_USER1_DATA            ADDR_TASK_USER1               /* |  DATA  | <-- 0x801000 */
#define ADDR_TASK_USER1_CODE            ADDR_TASK_USER1 + PAGE_SIZE   /* |  CODE  | <-- 0x802000 */
#define ADDR_TASK_USER1_STACK_USER      ADDR_TASK_USER1 + 2*PAGE_SIZE /* | USTACK | <-- 0x803000 */
#define ADDR_TASK_USER1_STACK_KERNEL    ADDR_TASK_USER1 + 3*PAGE_SIZE /* | KSTACK | <-- 0x804000 */

/* Address of task user in memory */
#define ADDR_TASK_USER2                 0x806000                      /*   TASK 2   */
#define ADDR_TASK_USER2_DATA            ADDR_TASK_USER1_DATA          /* |  DATA  | <-- 0x801000 */
#define ADDR_TASK_USER2_CODE            ADDR_TASK_USER2 + PAGE_SIZE   /* |  CODE  | <-- 0x807000 */
#define ADDR_TASK_USER2_STACK_USER      ADDR_TASK_USER2 + 2*PAGE_SIZE /* | USTACK | <-- 0x808000 */
#define ADDR_TASK_USER2_STACK_KERNEL    ADDR_TASK_USER2 + 3*PAGE_SIZE /* | KSTACK | <-- 0x809000 */



#endif