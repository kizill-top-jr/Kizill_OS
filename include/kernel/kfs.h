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
};

void kfs_init(void);
int kfs_mount(void);

#endif
