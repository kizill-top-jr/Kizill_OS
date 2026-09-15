#ifndef KERNEL_TYPES_H
#define KERNEL_TYPES_H

/* Fixed-width types. We don't pull in stdint.h to keep the kernel freestanding. */

/* Short names */
typedef unsigned char       u8;
typedef signed char         s8;
typedef unsigned short      u16;
typedef signed short        s16;
typedef unsigned int        u32;
typedef signed int          s32;
typedef unsigned long long  u64;
typedef signed long long    s64;

/* stdint-style aliases so code stays portable */
typedef u8   uint8_t;
typedef s8   int8_t;
typedef u16  uint16_t;
typedef s16  int16_t;
typedef u32  uint32_t;
typedef s32  int32_t;
typedef u64  uint64_t;
typedef s64  int64_t;

typedef u64 size_t;
typedef s64 ssize_t;
typedef u64 uintptr_t;

#define NULL ((void *)0)

/* Common limits */
#define U8_MAX  0xFF
#define U16_MAX 0xFFFF
#define U32_MAX 0xFFFFFFFFu
#define U64_MAX 0xFFFFFFFFFFFFFFFFull

#endif
