#ifndef KERNEL_ATA_H
#define KERNEL_ATA_H

#include <kernel/types.h>

// Primary Bus I/O ports
#define ATA_PRIMARY_DATA          0x1F0
#define ATA_PRIMARY_ERROR         0x1F1
#define ATA_PRIMARY_SECCOUNT      0x1F2
#define ATA_PRIMARY_LBA_LO        0x1F3
#define ATA_PRIMARY_LBA_MID       0x1F4
#define ATA_PRIMARY_LBA_HI        0x1F5
#define ATA_PRIMARY_DRIVE_HEAD    0x1F6
#define ATA_PRIMARY_COMMAND       0x1F7
#define ATA_PRIMARY_STATUS        0x1F7

// Status register bits
#define ATA_SR_BSY  0x80 // Busy
#define ATA_SR_DRDY 0x40 // Drive ready
#define ATA_SR_DF   0x20 // Drive fault
#define ATA_SR_DSC  0x10 // Drive seek complete
#define ATA_SR_DRQ  0x08 // Data request ready
#define ATA_SR_CORR 0x04 // Corrected data
#define ATA_SR_IDX  0x02 // Index
#define ATA_SR_ERR  0x01 // Error

// Commands
#define ATA_CMD_READ_PIO  0x20
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_IDENTIFY  0xEC

// // ATA driver public API
void ata_init(void);
int ata_read_sector(u32 lba, u8 *buffer);
int ata_write_sector(u32 lba, const u8 *buffer);

#endif
