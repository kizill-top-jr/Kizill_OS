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
    for (u64 i = 0; i < sizeof(sb); i++) sector[i] = ((u8*)&sb)[i];

    if (ata_write_sector(1, sector) == 0) {
        serial_puts("kfs: superblock written\n");
    } else {
        serial_puts("kfs: superblock write FAILED\n"); // nadeus takogo ne budet
    }
}

int kfs_mount(void) {
    u8 sector[512];
    if (ata_read_sector(1, sector) != 0) {
        serial_puts("kfs: mount fail (ata read)\n");
        return -1;
    }

    struct kfs_superblock *sb = (struct kfs_superblock *)sector;

    if (sb->magic != KFS_MAGIC) {
        serial_puts("kfs: mount fail (bad magic)\n");
        return -1;
    }

    serial_puts("kfs: mounted. block_size=");
    serial_dec(sb->block_size);
    serial_puts(" inodes=");
    serial_dec(sb->inode_count);
    serial_puts("\n");

    return 0;
}
