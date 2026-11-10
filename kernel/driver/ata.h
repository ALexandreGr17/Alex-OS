#ifndef ATA_H
#define ATA_H

#include <arch/pci/pci.h>

int init_ata(pci_device_t* master_dev);
int ata_read_28(uint32_t lba, uint16_t* buffer);
int ata_write_28(uint32_t lba, uint16_t* buffer);

#endif
