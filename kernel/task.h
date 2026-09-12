#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define MAX_TASKS 8
#define STACK_SIZE 4096   // 16 KB per task in uint32_t units

typedef struct {
    uint32_t esp;
    uint32_t pid;
    char name[32];
} task_t;

void init_scheduler(void);
void add_task(void (*entry)(void), const char *name);
void schedule(void);

#endif
