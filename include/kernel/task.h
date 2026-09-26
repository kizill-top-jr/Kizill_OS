#ifndef KERNEL_TASK_H
#define KERNEL_TASK_H

#include <kernel/types.h>

#define MAX_TASKS       8
#define TASK_STACK_SIZE 16384

typedef struct {
    u64  rsp;
    u64  pid;
    int  is_user;      // 1 = user-mode task (ring 3), 0 = kernel task
    char name[32];
} task_t;

void  scheduler_init(void);
int   task_create(void (*entry)(void), const char *name);
int   task_create_user(void (*entry)(void), const char *name);
u64   scheduler_tick(u64 current_rsp);

#endif
