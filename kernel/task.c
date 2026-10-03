#include <kernel/task.h>
#include <kernel/fb.h>
#include <kernel/gdt.h>
#include <kernel/paging.h>

extern volatile u64 g_ticks;

static task_t tasks[MAX_TASKS];
static int    task_count = 0;
static int    current    = 0;

static u8 stacks[MAX_TASKS][TASK_STACK_SIZE] __attribute__((aligned(16)));

// find a free slot (state == TASK_DEAD) between 1 and MAX_TASKS-1
static int alloc_slot(void) {
    for (int i = 1; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_DEAD) return i;
    }
    return -1;
}

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

static void wake_waiters_of(u64 dead_pid) {
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].state == TASK_BLOCKED &&
            tasks[i].waiting_for_pid == (int)dead_pid) {
            tasks[i].state = TASK_READY;
            tasks[i].waiting_for_pid = -1;
        }
    }
}

static int switch_cr3_to(int next) {
    u64 new_cr3 = tasks[next].cr3;
    u64 want = (new_cr3 == 0) ? pml4_master() : new_cr3;

    u64 cur;
    asm volatile("mov %%cr3, %0" : "=r"(cur));
    cur &= ~0xFFFULL;

    if (cur != want) {
        asm volatile("mov %0, %%cr3" : : "r"(want) : "memory");
        return 1;
    }
    return 0;
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
    tasks[0].cr3 = 0;
    tasks[0].waiting_for_pid = -1;
    const char *n = "main";
    int i = 0;
    while (n[i] && i < 31) { tasks[0].name[i] = n[i]; i++; }
    tasks[0].name[i] = 0;

    // mark all other slots as free
    for (int i = 1; i < MAX_TASKS; i++) {
        tasks[i].state = TASK_DEAD;
        tasks[i].cr3 = 0;
    }
}

int task_create(void (*entry)(void), const char *name) {
    int id = alloc_slot();
    if (id < 0) return -1;
    if (id >= task_count) task_count = id + 1;

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
    tasks[id].cr3 = 0;
    tasks[id].waiting_for_pid = -1;

    int i = 0;
    while (name[i] && i < 31) { tasks[id].name[i] = name[i]; i++; }
    tasks[id].name[i] = 0;

    return id;
}

int task_create_user(void (*entry)(void), u64 user_stack,
                     u64 pml4_phys, const char *name) {
    int id = alloc_slot();
    if (id < 0) return -1;
    if (id >= task_count) task_count = id + 1;

    u64 stack_top = (u64)(stacks[id] + TASK_STACK_SIZE);
    stack_top &= ~0xFULL;
    u64 *sp = (u64 *)stack_top;

    *--sp = 0x23;
    *--sp = user_stack & ~0xFULL;   // align to 16 bytes
    *--sp = 0x202;
    *--sp = 0x1B;
    *--sp = (u64)entry;

    *--sp = 0;
    *--sp = 0x80;
    for (int i = 0; i < 15; i++) *--sp = 0;

    tasks[id].rsp = (u64)sp;
    tasks[id].pid = id + 1;
    tasks[id].parent_pid = tasks[current].pid;
    tasks[id].state = TASK_READY;
    tasks[id].exit_code = 0;
    tasks[id].is_user = 1;
    tasks[id].cr3 = pml4_phys;
    tasks[id].waiting_for_pid = -1;

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
    g_ticks++;

    tasks[current].rsp = current_rsp;
    if (tasks[current].state == TASK_RUNNING)
        tasks[current].state = TASK_READY;

    u64 rsp = pick_and_switch();
    switch_cr3_to(current);
    return rsp;
}

u64 task_yield(u64 current_rsp) {
    tasks[current].rsp = current_rsp;
    if (tasks[current].state == TASK_RUNNING)
        tasks[current].state = TASK_READY;

    u64 rsp = pick_and_switch();
    switch_cr3_to(current);
    return rsp;
}

u64 task_exit_current(int code) {
    tasks[current].state = TASK_ZOMBIE;
    tasks[current].exit_code = code;

    printk_color("[exit] pid=", FB_YELLOW);
    printk_dec(tasks[current].pid);
    printk(" code=");
    printk_dec((u64)(code < 0 ? 0 : code));
    printk(" name=");
    printk(tasks[current].name);
    printk("\n");

    u64 dead_pid = tasks[current].pid;
    wake_waiters_of(dead_pid);

    u64 rsp = pick_and_switch();
    switch_cr3_to(current);
    return rsp;
}

u64 task_block_current(void) {
    tasks[current].state = TASK_BLOCKED;
    u64 rsp = pick_and_switch();
    switch_cr3_to(current);
    return rsp;
}

int task_current_pid(void) {
    return (int)tasks[current].pid;
}

int task_try_reap(u64 parent_pid, int want_pid, int *out_code) {
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].parent_pid != parent_pid) continue;
        if (want_pid >= 0 && (int)tasks[i].pid != want_pid) continue;
        if (tasks[i].state != TASK_ZOMBIE) continue;

        if (out_code) *out_code = tasks[i].exit_code;

        if (tasks[i].cr3) {
            pml4_destroy(tasks[i].cr3);
            tasks[i].cr3 = 0;
        }
        tasks[i].state = TASK_DEAD;
        tasks[i].parent_pid = 0;

        return 1;
    }
    return 0;
}

u64 task_get_cr3(int idx) {
    if (idx < 0 || idx >= task_count) return 0;
    return tasks[idx].cr3;
}

void task_set_cr3(int idx, u64 cr3) {
    if (idx < 0 || idx >= task_count) return;
    tasks[idx].cr3 = cr3;
}

int task_get_info(int idx, task_info_t *out) {
    if (idx < 0 || idx >= task_count) return -1;
    if (tasks[idx].state == TASK_DEAD) return -1;

    out->pid = tasks[idx].pid;
    out->parent_pid = tasks[idx].parent_pid;
    out->state = tasks[idx].state;
    for (int i = 0; i < 32; i++) out->name[i] = tasks[idx].name[i];

    return 0;
}

int task_alive_count(void) {
    int n = 0;
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].state != TASK_DEAD) n++;
    }
    return n;
}
