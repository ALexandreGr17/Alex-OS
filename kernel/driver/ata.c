#include "ata.h"
#include "arch/io.h"
#include "arch/pci/pci.h"
#include <stdint.h>
#include <logs/log.h>

#define MASTER_PORT_BASE 0x1F0
#define SLAVE_PORT_BASE 0x170

#define ATA_TIMEOUT 1000000

#define MASTER_PORT(register) (MASTER_PORT_BASE + register)
#define SLAVE_PORT(register) (SLAVE_PORT_BASE + register)

enum ATA_REGISTER_OFFSET {
    Data                = 0,
    Error_Feature       = 1,
    SectorCount         = 2,
    LBA_Low             = 3,
    LBA_Mid             = 4,
    LBA_High            = 5,
    Drive_Head          = 6,
    Stat_Cmd            = 7,
    Ctrl_1              = 0x206,
    Ctrl_2              = 0x207,
};

enum ATA_CMD {
    ATA_READ_SECTOR_28      = 0x20,
    ATA_WRITE_SECTOR_28     = 0x30,
    ATA_CACHE_FLUSH         = 0xE7,
    ATA_IDENTIFY            = 0xEC,
};

enum ATA_STATUS {
    ATA_STATUS_ERR  = (1 << 0),
    ATA_STATUS_IDX  = (1 << 1),
    ATA_STATUS_CORR = (1 << 2),
    ATA_STATUS_DRQ  = (1 << 3),
    ATA_STATUS_SRV  = (1 << 4),
    ATA_STATUS_DF   = (1 << 5),
    ATA_STATUS_RDY  = (1 << 6),
    ATA_STATUS_BSY  = (1 << 7),

    ATA_STATUS_NO_FLAG = 0,
};

void ata_wait() {
    for (int i = 0; i < 15; i++) {
        io_in8(MASTER_PORT(Ctrl_1));
    }
}

int init_ata(pci_device_t* master_dev) {
    if (master_dev == 0) return -1;
    uint16_t cmd = pci_read_16(master_dev, 4);

    if ((cmd & 1) == 0) {
        pci_write_16(master_dev, 4, cmd | 1);
    }

    cmd = pci_read_16(master_dev, 0x04);
    logf("PCI Command: 0x%x\n", cmd);


    io_out8(MASTER_PORT(Drive_Head), 0xA0);
    ata_wait();

    io_out8(MASTER_PORT(LBA_Low), 0);
    io_out8(MASTER_PORT(LBA_Mid), 0);
    io_out8(MASTER_PORT(LBA_High), 0);

    io_out8(MASTER_PORT(Stat_Cmd), ATA_IDENTIFY);
    ata_wait();

    uint8_t status = io_in8(MASTER_PORT(Stat_Cmd));
    while (status & 0x80) {
        ata_wait();
        status = io_in8(MASTER_PORT(Stat_Cmd));
    }

    uint8_t check_high = io_in8(MASTER_PORT(LBA_High));
    uint8_t check_mid = io_in8(MASTER_PORT(LBA_Mid));

    logf("ATA master status: 0x%x\n", status);
    logf("ATA check high: 0x%x, check_mid: 0x%x\n", check_high, check_mid);

    if (check_high || check_mid) {
        logf("Drive is not ATA, lba high: %x, lba mid: %x\n", check_high, check_mid);
        return -1;
    }

    status = io_in8(MASTER_PORT(Stat_Cmd));
    while (!(status & 0x8) && !(status & 0x1)) {
        ata_wait();
        status = io_in8(MASTER_PORT(Stat_Cmd));
    }

    if (status & 0x1) {
        logf("Error while initializing the drive\n");
        return -1;
    }

    uint16_t identify_data[256] = { 0 };

    for (int i = 0; i < 256; i++) {
        identify_data[i] = io_in16(MASTER_PORT(Data));
    }

    uint32_t sector_count = (uint32_t)identify_data[60] | ((uint32_t)identify_data[61] << 16);

    logf("Disk sector: %d\n", sector_count);
    logf("Disk size: %d MiB\n", (sector_count * 512ULL) / (1024 * 1024));

    logf("Drive initialized status: %x\n", status);
    return 0;
}

static int ata_poll(uint8_t mask)
{
    for (uint32_t i = 0; i < ATA_TIMEOUT; i++) {
        uint8_t status = io_in8(MASTER_PORT(Stat_Cmd));

        if (status == 0 || status == 0xFF)
            return -1;

        if (status & ATA_STATUS_BSY)
            continue;

        if (status & (ATA_STATUS_ERR | ATA_STATUS_DF))
            return -1;

        if ((status & mask) == mask)
            return 0;
    }

    return -1;
}

int ata_read_28(uint32_t lba, uint16_t* buffer) {
    if (lba >= (1u << 28)) {
        return -1;
    }

    if (ata_poll(ATA_STATUS_NO_FLAG)) {
        logf("Error in ata read 28\n");
        return -1;
    }

    io_out8(MASTER_PORT(Drive_Head), 0xE0 | ((lba >> 24) & 0x0F));

    io_out8(MASTER_PORT(SectorCount), 1);

    io_out8(MASTER_PORT(LBA_Low), (uint8_t)lba);
    io_out8(MASTER_PORT(LBA_Mid), (uint8_t)(lba >> 8));
    io_out8(MASTER_PORT(LBA_High), (uint8_t)(lba >> 16));

    io_out8(MASTER_PORT(Stat_Cmd), ATA_READ_SECTOR_28);

    ata_poll(ATA_STATUS_DRQ);

    for (int i = 0; i < 256; i++) {
        buffer[i] = io_in16(MASTER_PORT(Data));
    }

    return 0;
}

int ata_write_28(uint32_t lba, uint16_t* buffer) {
    if (lba >= (1u << 28)) {
        return -1;
    }

    if (ata_poll(ATA_STATUS_NO_FLAG)) {
        logf("Error in ata write 28\n");
        return -1;
    }

    io_out8(MASTER_PORT(Drive_Head), 0xE0 | ((lba >> 24) & 0x0F));

    io_out8(MASTER_PORT(SectorCount), 1);

    io_out8(MASTER_PORT(LBA_Low), (uint8_t)lba);
    io_out8(MASTER_PORT(LBA_Mid), (uint8_t)(lba >> 8));
    io_out8(MASTER_PORT(LBA_High), (uint8_t)(lba >> 16));

    io_out8(MASTER_PORT(Stat_Cmd), ATA_WRITE_SECTOR_28);

    ata_poll(ATA_STATUS_DRQ);

    for (int i = 0; i < 256; i++) {
        io_out16(MASTER_PORT(Data), buffer[i]);
    }

    if (ata_poll(ATA_STATUS_NO_FLAG)) {
        logf("Error in ata write 28\n");
        return -1;
    }

    return 0;
}
