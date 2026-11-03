#include <kernel/kfs.h>
#include <kernel/ata.h>
#include <kernel/serial.h>

void kfs_init(void) {
    struct kfs_superblock sb;
    sb.magic = KFS_MAGIC;
    sb.block_size = 4096;
    sb.inode_count = 0;
    sb.free_blocks = 0;

    u8 sector[512];
    for (int i = 0; i < 512; i++) sector[i] = 0;
    for (int i = 0; i < sizeof(sb); i++) sector[i] = ((u8*)&sb)[i];

    if (ata_write_sector(1, sector) == 0) {
        serial_puts("kfs: superblock written\n");
    } else {
        serial_puts("kfs: superblock write FAILED\n"); // nadeus takogo ne budet
    }
}
