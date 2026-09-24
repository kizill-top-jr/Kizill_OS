arch/x86_64/pic/

8259 PIC remap. Standard ICW sequence — remaps IRQ0..15 to vectors 32..47.
All IRQs masked at init, unmasked as drivers come up.

    pic_remap() — call once at boot, before sti.

    pic_unmask_timer() — unmask IRQ0 only.

Gotcha: EOI goes only to master PIC unless you actually handle a
slave IRQ. Don't spam out 0xA0, al.
