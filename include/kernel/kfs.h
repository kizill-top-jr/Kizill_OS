#ifndef KERNEL_KFS_H
#define KERNEL_KFS_H

#include <kernel/types.h>

#define KFS_MAGIC 0x4B465331 // "KFS1"
// haha magic tipo MAGIC
struct kfs_superblock {
    u32 magic;
    u32 block_size;
    u32 inode_count;
    u32 free_blocks;
    u32 inode_table_start;
    u32 bitmap_start;
    u32 data_start;
};

#define KFS_TYPE_FREE 0
#define KFS_TYPE_FILE 1
#define KFS_TYPE_DIR  2

struct kfs_inode {
    u32 magic;      // 0x494E4F44 "INOD"
    u32 type;       // 0=free, 1=file, 2=dir
    u32 size;
    u32 block[12];  // 4KB
};

u32 kfs_alloc_block(void);
void kfs_free_block(u32 block);

void kfs_init(void);
int kfs_mount(void);

#endif
