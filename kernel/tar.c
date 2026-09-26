#include <kernel/tar.h>

// tar header is 512 bytes; file data is padded to 512
#define TAR_BLOCK 512

static u64 octal(const char *s, u64 len) {
    u64 v = 0;
    for (u64 i = 0; i < len; i++) {
        if (s[i] < '0' || s[i] > '7') break;
        v = v * 8 + (s[i] - '0');
    }
    return v;
}

static u64 strlen_(const char *s) {
    u64 n = 0;
    while (s[n]) n++;
    return n;
}

const tar_entry_t *tar_next(const u8 *base, u64 size, const tar_entry_t *prev) {
    static tar_entry_t entry;

    u64 off = 0;
    if (prev) {
        // advance past current entry
        off = (u64)(prev->data - base) + ((prev->size + TAR_BLOCK - 1) / TAR_BLOCK) * TAR_BLOCK;
    }

    while (off + TAR_BLOCK <= size) {
        const u8 *hdr = base + off;

        // empty block (end of archive)
        if (hdr[0] == 0) return 0;

        u64 fsize = octal((const char *)hdr + 124, 12);
        const char *name = (const char *)hdr;

        // skip directories (name ends with /)
        u64 nlen = strlen_(name);
        if (nlen > 0 && name[nlen - 1] == '/') {
            off += TAR_BLOCK;
            continue;
        }

        entry.name = name;
        entry.size = fsize;
        entry.data = base + off + TAR_BLOCK;
        return &entry;
    }
    return 0;
}

const tar_entry_t *tar_find(const u8 *base, u64 size, const char *name) {
    const tar_entry_t *e = 0;
    while ((e = tar_next(base, size, e))) {
        u64 i = 0;
        while (e->name[i] && name[i] && e->name[i] == name[i]) i++;
        if (e->name[i] == 0 && name[i] == 0) return e;
    }
    return 0;
}
