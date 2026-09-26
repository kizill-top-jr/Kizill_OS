#include <kernel/task.h>
#include <kernel/fb.h>
#include <kernel/gdt.h>

static task_t tasks[MAX_TASKS];
static int    task_count = 0;
static int    current    = 0;

static u8 stacks[MAX_TASKS][TASK_STACK_SIZE] __attribute__((aligned(16)));

void scheduler_init(void) {
    task_count = 1;
    current = 0;
    tasks[0].pid = 0;
    tasks[0].rsp = 0;

    const char *n = "main";
    int i = 0;
    while (n[i] && i < 31) { tasks[0].name[i] = n[i]; i++; }
    tasks[0].name[i] = 0;
}

int task_create(void (*entry)(void), const char *name) {
    if (task_count >= MAX_TASKS) return -1;
    int id = task_count++;

    u64 stack_top = (u64)(stacks[id] + TASK_STACK_SIZE);
    stack_top &= ~0xFULL;
    u64 *sp = (u64 *)stack_top;

    *--sp = 0x10;
    *--sp = stack_top;
    *--sp = 0x202;
    *--sp = 0x08;
    *--sp = (u64)entry;

    *--sp = 0;
    *--sp = 32;

    for (int i = 0; i < 15; i++) *--sp = 0;

    tasks[id].rsp = (u64)sp;
    tasks[id].pid = id + 1;

    int i = 0;
    while (name[i] && i < 31) { tasks[id].name[i] = name[i]; i++; }
    tasks[id].name[i] = 0;

    return id;
}

u64 scheduler_tick(u64 current_rsp) {
    tasks[current].rsp = current_rsp;

    int next = (current + 1) % task_count;
    current = next;

    // set kernel stack for the next task -- used when CPU switches
    // from ring 3 to ring 0 on interrupt
    u64 kstack_top = (u64)(stacks[next] + TASK_STACK_SIZE);
    tss_set_rsp0(kstack_top);

    return tasks[next].rsp;
}
int task_create_user(void (*entry)(void), const char *name) {
    if (task_count >= MAX_TASKS) return -1;
    int id = task_count++;

    u64 stack_top = (u64)(stacks[id] + TASK_STACK_SIZE);
    stack_top &= ~0xFULL;
    u64 *sp = (u64 *)stack_top;

    // frame for iretq into ring 3:
    //   SS:RSP:RFLAGS:CS:RIP
    *--sp = 0x23;                  // SS = user data | RPL 3
    *--sp = 0x501000;              // RSP = user stack top
    *--sp = 0x202;                 // RFLAGS (IF=1)
    *--sp = 0x1B;                  // CS = user code | RPL 3
    *--sp = (u64)entry;            // RIP

    *--sp = 0;                     // error code
    *--sp = 32;                    // vector

    for (int i = 0; i < 15; i++) *--sp = 0;

    tasks[id].rsp = (u64)sp;
    tasks[id].pid = id + 1;
    tasks[id].is_user = 1;

    int i = 0;
    while (name[i] && i < 31) { tasks[id].name[i] = name[i]; i++; }
    tasks[id].name[i] = 0;

    return id;
}
