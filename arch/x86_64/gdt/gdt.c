#include <kernel/gdt.h>

struct gdt_entry {
    u16 limit_low; u16 base_low; u8 base_mid;
    u8 access; u8 granularity; u8 base_high;
} __attribute__((packed));

struct tss_entry {
    u32 reserved0;
    u64 rsp0; u64 rsp1; u64 rsp2;
    u64 reserved1;
    u64 ist[7];
    u64 reserved2;
    u16 reserved3;
    u16 iomap_base;
} __attribute__((packed));

struct gdt_ptr { u16 limit; u64 base; } __attribute__((packed));

static struct gdt_entry gdt[7] __attribute__((section(".data")));
static struct tss_entry tss   __attribute__((section(".data")));
static struct gdt_ptr   gdtr  __attribute__((section(".data")));

extern void gdt_load(struct gdt_ptr *ptr);
extern void tss_load(void);

static void set_gate(int i, u32 base, u32 limit, u8 access, u8 gran) {
    gdt[i].base_low    = base & 0xFFFF;
    gdt[i].base_mid    = (base >> 16) & 0xFF;
    gdt[i].base_high   = (base >> 24) & 0xFF;
    gdt[i].limit_low   = limit & 0xFFFF;
    gdt[i].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[i].access      = access;
}

static void set_tss_gate(int i, u64 base, u32 limit) {
    u8 *lo = (u8 *)&gdt[i];
    u8 *hi = (u8 *)&gdt[i + 1];
    lo[0] = limit & 0xFF; lo[1] = (limit >> 8) & 0xFF;
    lo[2] = base & 0xFF;  lo[3] = (base >> 8) & 0xFF;
    lo[4] = (base >> 16) & 0xFF; lo[5] = 0x89;
    lo[6] = (limit >> 16) & 0x0F; lo[7] = (base >> 24) & 0xFF;
    hi[0] = (base >> 32) & 0xFF; hi[1] = (base >> 40) & 0xFF;
    hi[2] = (base >> 48) & 0xFF; hi[3] = (base >> 56) & 0xFF;
    hi[4] = hi[5] = hi[6] = hi[7] = 0;
}

void tss_set_rsp0(u64 rsp0) { tss.rsp0 = rsp0; }

void gdt_init(void) {
    u8 *p = (u8 *)&tss;
    for (u64 i = 0; i < sizeof(tss); i++) p[i] = 0;
    tss.iomap_base = 0xFFFF;

    set_gate(0, 0, 0, 0x00, 0x00);
    set_gate(1, 0, 0xFFFFF, 0x9A, 0xA0);
    set_gate(2, 0, 0xFFFFF, 0x92, 0xC0);
    set_gate(3, 0, 0xFFFFF, 0xFA, 0xA0);
    set_gate(4, 0, 0xFFFFF, 0xF2, 0xC0);
    set_tss_gate(5, (u64)&tss, sizeof(tss) - 1);

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base  = (u64)&gdt;

    __asm__ __volatile__("lgdt %0" : : "m"(gdtr) : "memory");
    gdt_load(&gdtr);
    tss_load();
}
