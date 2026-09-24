#ifndef KERNEL_TASK_H
#define KERNEL_TASK_H

#include <kernel/types.h>

#define MAX_TASKS       8
#define TASK_STACK_SIZE 4096

typedef struct {
    u64  rsp;       // saved stack pointer
    u64  pid;
    char name[32];
} task_t;

void  scheduler_init(void);
int   task_create(void (*entry)(void), const char *name);
u64   scheduler_tick(u64 current_rsp);

#endif
