// task.c - cooperative round-robin scheduler
#include <stdint.h>
#include "task.h"

static task_t tasks[MAX_TASKS];
static int task_count = 0;
static int current = 0;
static int next_pid = 1;

static uint32_t stacks[MAX_TASKS][STACK_SIZE];

extern void switch_task(uint32_t *old_esp, uint32_t new_esp);

static void copy_name(char *dst, const char *src) {
    int i;
    for (i = 0; i < 31 && src[i]; i++) dst[i] = src[i];
    dst[i] = '\0';
}

void init_scheduler(void) {
    // Slot 0 is reserved for the main/idle context.
    // Its esp is filled on the first switch_task() call.
    task_count = 1;
    current = 0;
    next_pid = 1;
    copy_name(tasks[0].name, "main");
    tasks[0].pid = next_pid++;
}

void add_task(void (*entry)(void), const char *name) {
    if (task_count >= MAX_TASKS) return;

    task_t *t = &tasks[task_count];
    t->pid = next_pid++;
    copy_name(t->name, name);

    // Build initial stack.
    // Layout expected by popf/popa/ret in switch_task:
    //   [sp+0]  EFLAGS
    //   [sp+4]  EDI
    //   [sp+8]  ESI
    //   [sp+12] EBP
    //   [sp+16] ESP (dummy)
    //   [sp+20] EBX
    //   [sp+24] EDX
    //   [sp+28] ECX
    //   [sp+32] EAX
    //   [sp+36] EIP
    // Since we use *--sp, we push in reverse order.
    uint32_t *sp = &stacks[task_count][STACK_SIZE];

    *--sp = (uint32_t)entry;   // EIP
    *--sp = 0;                 // EAX
    *--sp = 0;                 // ECX
    *--sp = 0;                 // EDX
    *--sp = 0;                 // EBX
    *--sp = 0;                 // ESP (dummy)
    *--sp = 0;                 // EBP
    *--sp = 0;                 // ESI
    *--sp = 0;                 // EDI
    *--sp = 0x200;             // EFLAGS (IF=1)

    t->esp = (uint32_t)sp;
    task_count++;
}

void schedule(void) {
    if (task_count < 2) return;

    int prev = current;
    current = (current + 1) % task_count;
    if (current == prev) return;

    switch_task(&tasks[prev].esp, tasks[current].esp);
}
