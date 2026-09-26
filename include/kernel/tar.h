#ifndef KERNEL_TAR_H
#define KERNEL_TAR_H

#include <kernel/types.h>

typedef struct {
    const char *name;
    const u8   *data;
    u64         size;
} tar_entry_t;

// iterate through tar. pass prev = NULL to get first.
// returns NULL when done.
const tar_entry_t *tar_next(const u8 *tar_base, u64 tar_size, const tar_entry_t *prev);

// find file by name, or NULL
const tar_entry_t *tar_find(const u8 *tar_base, u64 tar_size, const char *name);

#endif
