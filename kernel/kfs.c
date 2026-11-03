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
u32 kfs_alloc_block(void) {
    u8 bitmap_sector[512];

    // bitmap is 8 sectors (4096 bytes = 32768 bits = 32768 blocks)
    for (u32 sec = 0; sec < 8; sec++) {
        if (ata_read_sector(10 + sec, bitmap_sector) != 0) return 0;

        for (u32 i = 0; i < 512; i++) {
            if (bitmap_sector[i] != 0xFF) { // if not all bits are 1
                for (u32 bit = 0; bit < 8; bit++) {
                    if (!(bitmap_sector[i] & (1 << bit))) {
                        // found free block
                        bitmap_sector[i] |= (1 << bit);
                        ata_write_sector(10 + sec, bitmap_sector);

                        u32 block_num = (sec * 512 * 8) + (i * 8) + bit;
                        return 20 + block_num; // data starts at LBA 20
                    }
                }
            }
        }
    }
    return 0; // no free blocks
}

void kfs_free_block(u32 block) {
    u32 block_num = block - 20;
    u32 sec = block_num / (512 * 8);
    u32 byte_idx = (block_num / 8) % 512;
    u32 bit = block_num % 8;

    u8 bitmap_sector[512];
    if (ata_read_sector(10 + sec, bitmap_sector) != 0) return;

    bitmap_sector[byte_idx] &= ~(1 << bit);
    ata_write_sector(10 + sec, bitmap_sector);
}

u32 kfs_alloc_inode(void) {
    u8 sector[512];
    // inode table starts at LBA 2, 64 inodes * 64 bytes = 4096 bytes = 8 sectors
    for (u32 sec = 0; sec < 8; sec++) {
        if (ata_read_sector(2 + sec, sector) != 0) return 0;

        struct kfs_inode *inodes = (struct kfs_inode *)sector;
        u32 count = 512 / sizeof(struct kfs_inode);

        for (u32 i = 0; i < count; i++) {
            if (inodes[i].type == KFS_TYPE_FREE) {
                u32 inode_num = (sec * count) + i;
                return inode_num;
            }
        }
    }
    return 0; // no free inodes
}

#include <kernel/heap.h> // maybe not needed, but good to have

int kfs_create(const char *name) {
    (void)name; // for now, just alloc
    u32 inode_num = kfs_alloc_inode();
    if (inode_num == 0) return -1;

    u8 sector[512];
    u32 sec = inode_num / (512 / sizeof(struct kfs_inode));
    u32 idx = inode_num % (512 / sizeof(struct kfs_inode));

    if (ata_read_sector(2 + sec, sector) != 0) return -1;
    struct kfs_inode *inodes = (struct kfs_inode *)sector;

    inodes[idx].magic = 0x494E4F44;
    inodes[idx].type = KFS_TYPE_FILE;
    inodes[idx].size = 0;
    for (int i = 0; i < 12; i++) inodes[idx].block[i] = 0;

    if (ata_write_sector(2 + sec, sector) != 0) return -1;

    serial_puts("kfs: created file (inode ");
    serial_dec(inode_num);
    serial_puts(")\n");
    return 0;
}
int kfs_write(const char *name, const u8 *data, u32 size) {
    (void)name; // Ignored for now, just writing to inode 1

    u32 inode_num = 1;
    u8 sector[512];
    u32 sec = inode_num / (512 / sizeof(struct kfs_inode));
    u32 idx = inode_num % (512 / sizeof(struct kfs_inode));

    if (ata_read_sector(2 + sec, sector) != 0) return -1;
    struct kfs_inode *inodes = (struct kfs_inode *)sector;

    if (inodes[idx].type != KFS_TYPE_FILE) {
        serial_puts("kfs: write fail (not a file)\n");
        return -1;
    }

    // Max 1 block (4096 bytes) for now
    if (size > 4096) size = 4096;

    // Allocate data block
    u32 data_block = kfs_alloc_block();
    if (data_block == 0) return -1;

    // Write data to disk
    u8 data_sector[512];
    u32 sectors_needed = (size + 511) / 512;

    for (u32 s = 0; s < sectors_needed; s++) {
        for (int i = 0; i < 512; i++) data_sector[i] = 0;
        u32 chunk = size - (s * 512);
        if (chunk > 512) chunk = 512;
        for (u32 i = 0; i < chunk; i++) data_sector[i] = data[s * 512 + i];
        ata_write_sector(data_block + s, data_sector);
    }

    // Update inode
    inodes[idx].size = size;
    inodes[idx].block[0] = data_block;

    if (ata_write_sector(2 + sec, sector) != 0) return -1;

    serial_puts("kfs: wrote ");
    serial_dec(size);
    serial_puts(" bytes to inode ");
    serial_dec(inode_num);
    serial_puts("\n");
    return 0;
}
