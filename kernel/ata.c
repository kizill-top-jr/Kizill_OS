#include <kernel/ata.h>
#include <kernel/serial.h>
#include <kernel/fb.h>

static inline void outb(u16 port, u8 val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// wait 400ns for drive to update status
static void ata_wait_400ns(void) {
    // reading the status port 4 times takes roughly 400ns
    inb(ATA_PRIMARY_STATUS);
    inb(ATA_PRIMARY_STATUS);
    inb(ATA_PRIMARY_STATUS);
    inb(ATA_PRIMARY_STATUS);
}

void ata_init(void) {
    serial_puts("ata: probing primary master...\n");

    // select drive 0 (master), LBA mode
    outb(ATA_PRIMARY_DRIVE_HEAD, 0xA0);
    ata_wait_400ns();

    serial_puts("ata: checking status\n");
    // check if there is a floating bus (no drive connected)
    if (inb(ATA_PRIMARY_STATUS) == 0xFF) {
        serial_puts("ata: no drive found (floating bus)\n");
        return;
    }

    // send IDENTIFY command
    outb(ATA_PRIMARY_SECCOUNT, 0);
    outb(ATA_PRIMARY_LBA_LO, 0);
    outb(ATA_PRIMARY_LBA_MID, 0);
    outb(ATA_PRIMARY_LBA_HI, 0);
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_IDENTIFY);
    ata_wait_400ns();

    serial_puts("ata: sent identify, waiting for BSY\n");
    // check if status is zero (drive does not exist)
    u8 status = inb(ATA_PRIMARY_STATUS);
    if (status == 0) {
        serial_puts("ata: drive does not exist\n");
        return;
    }

    // wait for BSY to clear
    u32 timeout = 1000000;
    while (status & ATA_SR_BSY) {
        status = inb(ATA_PRIMARY_STATUS);
        if (--timeout == 0) {
            serial_puts("ata: timeout waiting for BSY\n");
            return;
        }
    }

    // check if LBA_MID and LBA_HI are non-zero (means it's not ATA, maybe SATA or ATAPI)
    if (inb(ATA_PRIMARY_LBA_MID) != 0 || inb(ATA_PRIMARY_LBA_HI) != 0) {
        serial_puts("ata: not an ATA drive (maybe ATAPI/SATA)\n");
        return;
    }

    // wait for DRQ or ERR
    timeout = 1000000;
    while (1) {
        status = inb(ATA_PRIMARY_STATUS);
        if (status & ATA_SR_ERR) {
            serial_puts("ata: error during IDENTIFY\n");
            return;
        }
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) {
            break;
        }
        if (--timeout == 0) {
            serial_puts("ata: timeout waiting for DRQ\n");
            return;
        }
    }

    // read 256 words (512 bytes) of identify data
    u16 identify_data[256];
    for (int i = 0; i < 256; i++) {
        u16 data;
        asm volatile("inw %1, %0" : "=a"(data) : "d"((u16)ATA_PRIMARY_DATA));
        identify_data[i] = data;
    }

    serial_puts("ata: drive found and ready!\n");

    // print model name (words 27 to 46, swap bytes)
    serial_puts("ata: model: ");
    for (int i = 27; i <= 46; i++) {
        char c1 = (identify_data[i] >> 8) & 0xFF;
        char c2 = identify_data[i] & 0xFF;
        if (c1 >= 32 && c1 <= 126) serial_putc(c1);
        if (c2 >= 32 && c2 <= 126) serial_putc(c2);
    }
    serial_puts("\n");
}
