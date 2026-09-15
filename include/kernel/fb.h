#ifndef KERNEL_FB_H
#define KERNEL_FB_H

#include <kernel/types.h>

/* Limine gives us this. We keep only what we actually use. */
struct limine_framebuffer {
    void     *address;
    u64       width;
    u64       height;
    u64       pitch;
    u16       bpp;
    u8        memory_model;
    u8        red_mask_size;
    u8        red_mask_shift;
    u8        green_mask_size;
    u8        green_mask_shift;
    u8        blue_mask_size;
    u8        blue_mask_shift;
    u8        unused[7];
    u64       edid_size;
    void     *edid;
};

/* Colors: 0x00RRGGBB, alpha ignored (Limine gives us 32bpp) */
#define FB_BLACK   0x00000000
#define FB_WHITE   0x00FFFFFF
#define FB_RED     0x00C00000
#define FB_GREEN   0x0000A000
#define FB_BLUE    0x000000C0
#define FB_YELLOW  0x00C0C000
#define FB_CYAN    0x0000C0C0
#define FB_MAGENTA 0x00C000C0

void fb_init(struct limine_framebuffer *fb);
void fb_clear(u32 color);
void fb_put_pixel(u64 x, u64 y, u32 color);

void fb_draw_char(u64 x, u64 y, char c, u32 fg, u32 bg);
void fb_draw_string(u64 x, u64 y, const char *s, u32 fg, u32 bg);

/* Console-style output. Tracks its own cursor. */
void printk(const char *s);
void printk_color(const char *s, u32 fg);
void printk_hex(u64 v);
void printk_dec(u64 v);

#endif
