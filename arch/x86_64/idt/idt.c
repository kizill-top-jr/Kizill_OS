#include <kernel/idt.h>
#include <kernel/pic.h>
#include <kernel/fb.h>

struct idt_entry {
    u16 offset_low;
    u16 selector;
    u8  ist;
    u8  type_attr;
    u16 offset_mid;
    u32 offset_high;
    u32 reserved;
} __attribute__((packed));

struct idt_ptr {
    u16 limit;
    u64 base;
} __attribute__((packed));

static struct idt_entry idt[256] __attribute__((section(".data")));
static struct idt_ptr   idtr     __attribute__((section(".data")));

extern void isr_stub_0(void);
extern void isr_stub_13(void);
extern void isr_stub_14(void);
extern void isr_stub_8(void);
extern void isr_stub_default(void);
extern void isr_stub_timer(void);
extern void isr_stub_keyboard(void);

static void set_gate(int n, u64 handler, u16 sel, u8 ist, u8 type_attr) {
    idt[n].offset_low  = handler & 0xFFFF;
    idt[n].selector    = sel;
    idt[n].ist         = ist & 0x7;
    idt[n].type_attr   = type_attr;
    idt[n].offset_mid  = (handler >> 16) & 0xFFFF;
    idt[n].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[n].reserved    = 0;
}

void idt_init(void) {
    pic_remap();
    printk("pic: remapped\n");

    for (int i = 0; i < 256; i++)
        set_gate(i, (u64)isr_stub_default, 0x08, 0, 0x8E);

    set_gate(0,  (u64)isr_stub_0,        0x08, 0, 0x8E);
    set_gate(8,  (u64)isr_stub_8,        0x08, 0, 0x8E);
    set_gate(13, (u64)isr_stub_13,       0x08, 0, 0x8E);
    set_gate(14, (u64)isr_stub_14,       0x08, 0, 0x8E);
    set_gate(32, (u64)isr_stub_timer,    0x08, 0, 0x8E);
    set_gate(33, (u64)isr_stub_keyboard, 0x08, 0, 0x8E);

    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (u64)&idt;

    __asm__ __volatile__("lidt %0" : : "m"(idtr) : "memory");
    printk("idt: loaded at "); printk_hex(idtr.base); printk("\n");
}
