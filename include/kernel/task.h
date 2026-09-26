#ifndef KERNEL_TASK_H
#define KERNEL_TASK_H

#include <kernel/types.h>

#define MAX_TASKS       8
#define TASK_STACK_SIZE 16384

#define TASK_READY      0
#define TASK_RUNNING    1
#define TASK_BLOCKED    2
#define TASK_DEAD       3

typedef struct {
    u64  rsp;
    u64  pid;
    u64  parent_pid;
    int  state;
    int  exit_code;
    int  is_user;
    char name[32];
} task_t;

void  scheduler_init(void);
int   task_create(void (*entry)(void), const char *name);
int   task_create_user(void (*entry)(void), u64 user_stack, const char *name);
void  task_set_parent(int idx, u64 parent_pid);
u64   scheduler_tick(u64 current_rsp);

u64   task_exit_current(int code);
u64   task_block_current(void);
int   task_check_dead_child(void);
int   task_current_pid(void);

#endif
