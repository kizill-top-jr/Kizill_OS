#include <kernel/task.h>
#include <kernel/fb.h>
#include <kernel/gdt.h>

static task_t tasks[MAX_TASKS];
static int    task_count = 0;
static int    current    = 0;

static u8 stacks[MAX_TASKS][TASK_STACK_SIZE] __attribute__((aligned(16)));

static u64 pick_and_switch(void) {
    for (int i = 1; i <= task_count; i++) {
        int c = (current + i) % task_count;
        if (tasks[c].state == TASK_READY || tasks[c].state == TASK_RUNNING) {
            current = c;
            tasks[c].state = TASK_RUNNING;

            u64 kstack_top = (u64)(stacks[c] + TASK_STACK_SIZE);
            tss_set_rsp0(kstack_top);

            return tasks[c].rsp;
        }
    }
    for (;;) asm volatile("sti; hlt");
}

void scheduler_init(void) {
    task_count = 1;
    current = 0;
    tasks[0].pid = 0;
    tasks[0].parent_pid = 0;
    tasks[0].rsp = 0;
    tasks[0].state = TASK_RUNNING;
    tasks[0].exit_code = 0;
    tasks[0].is_user = 0;
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
    tasks[id].parent_pid = tasks[current].pid;
    tasks[id].state = TASK_READY;
    tasks[id].exit_code = 0;
    tasks[id].is_user = 0;

    int i = 0;
    while (name[i] && i < 31) { tasks[id].name[i] = name[i]; i++; }
    tasks[id].name[i] = 0;

    return id;
}

int task_create_user(void (*entry)(void), u64 user_stack, const char *name) {
    if (task_count >= MAX_TASKS) return -1;
    int id = task_count++;

    u64 stack_top = (u64)(stacks[id] + TASK_STACK_SIZE);
    stack_top &= ~0xFULL;
    u64 *sp = (u64 *)stack_top;

    *--sp = 0x23;                       // SS = user data | RPL 3
    *--sp = user_stack;                 // RSP = user stack (given)
    *--sp = 0x202;                      // RFLAGS
    *--sp = 0x1B;                       // CS = user code | RPL 3
    *--sp = (u64)entry;                 // RIP

    *--sp = 0;
    *--sp = 0x80;
    for (int i = 0; i < 15; i++) *--sp = 0;

    tasks[id].rsp = (u64)sp;
    tasks[id].pid = id + 1;
    tasks[id].parent_pid = tasks[current].pid;
    tasks[id].state = TASK_READY;
    tasks[id].exit_code = 0;
    tasks[id].is_user = 1;

    int i = 0;
    while (name[i] && i < 31) { tasks[id].name[i] = name[i]; i++; }
    tasks[id].name[i] = 0;

    return id;
}

void task_set_parent(int idx, u64 parent_pid) {
    if (idx < 0 || idx >= task_count) return;
    tasks[idx].parent_pid = parent_pid;
}

u64 scheduler_tick(u64 current_rsp) {
    tasks[current].rsp = current_rsp;
    if (tasks[current].state == TASK_RUNNING)
        tasks[current].state = TASK_READY;

    return pick_and_switch();
}

u64 task_exit_current(int code) {
    tasks[current].state = TASK_DEAD;
    tasks[current].exit_code = code;

    printk_color("[exit] pid=", FB_YELLOW);
    printk_dec(tasks[current].pid);
    printk(" code=");
    printk_dec((u64)(code < 0 ? 0 : code));
    printk(" name=");
    printk(tasks[current].name);
    printk("\n");

    u64 ppid = tasks[current].parent_pid;
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].pid == ppid && tasks[i].state == TASK_BLOCKED) {
            tasks[i].state = TASK_READY;
            break;
        }
    }

    return pick_and_switch();
}

u64 task_block_current(void) {
    tasks[current].state = TASK_BLOCKED;
    return pick_and_switch();
}

int task_check_dead_child(void) {
    u64 my_pid = tasks[current].pid;
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].parent_pid == my_pid && tasks[i].state == TASK_DEAD) {
            return tasks[i].exit_code;
        }
    }
    return -1;
}

int task_current_pid(void) {
    return (int)tasks[current].pid;
}
int task_get_current_state(void) {
    return tasks[current].state;
}

int task_get_current_pid_or_neg(void) {
    return (int)tasks[current].pid;
}
