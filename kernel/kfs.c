#include <kernel/kfs.h>
#include <kernel/ata.h>
#include <kernel/serial.h>

void kfs_init(void) {
    struct kfs_superblock sb;
    sb.magic = KFS_MAGIC;
    sb.block_size = 4096;
    sb.inode_count = 64;             // 64 inoda
    sb.free_blocks = 2500;           // 10MB / 4KB
    sb.inode_table_start = 2;        // LBA 2 (sector 2)
    sb.bitmap_start = 10;            // LBA 10
    sb.data_start = 20;              // LBA 20

    u8 sector[512];

    for (int i = 0; i < 512; i++) sector[i] = 0;
    for (u64 i = 0; i < sizeof(sb); i++) sector[i] = ((u8*)&sb)[i];
    ata_write_sector(1, sector);

    for (int i = 0; i < 512; i++) sector[i] = 0;
    for (int sec = 0; sec < 8; sec++) {
        ata_write_sector(sb.inode_table_start + sec, sector);
    }

    for (int sec = 0; sec < 8; sec++) {
        ata_write_sector(sb.bitmap_start + sec, sector);
    }

    struct kfs_inode root;
    root.magic = 0x494E4F44;
    root.type = KFS_TYPE_DIR;
    root.size = 0;
    for (int i = 0; i < 12; i++) root.block[i] = 0;

    for (int i = 0; i < 512; i++) sector[i] = 0;
    for (u64 i = 0; i < sizeof(root); i++) sector[i] = ((u8*)&root)[i];
    ata_write_sector(sb.inode_table_start, sector);

    serial_puts("kfs: formatted, inode table and bitmap initialized\n");
}

int kfs_mount(void) {
    u8 sector[512];
    if (ata_read_sector(1, sector) != 0) return -1;

    struct kfs_superblock *sb = (struct kfs_superblock *)sector;

    if (sb->magic != KFS_MAGIC) {
        serial_puts("kfs: mount fail (bad magic)\n");
        return -1;
    }

    // Save superblock fields before overwriting the buffer!
    u32 block_size = sb->block_size;
    u32 inode_count = sb->inode_count;
    u32 inode_table = sb->inode_table_start;

    // Read root inode
    if (ata_read_sector(inode_table, sector) != 0) return -1;
    struct kfs_inode *root = (struct kfs_inode *)sector;

    if (root->magic != 0x494E4F44) {
        serial_puts("kfs: mount fail (bad root inode)\n");
        return -1;
    }

    serial_puts("kfs: mounted. block_size=");
    serial_dec(block_size);
    serial_puts(" inodes=");
    serial_dec(inode_count);
    serial_puts(" root_type=");
    serial_dec(root->type);
    serial_puts("\n");

    return 0;
}
