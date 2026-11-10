#include "arch/pci/pci.h"
#include "arch/io.h"
#include <logs/log.h>
#include <stdint.h>

#define PCI_COMMAND 0xCF8
#define PCI_DATA 0xCFC

#define MAX_PCI_DEVICE 8 * 32 * 8

#define REG(x) x * sizeof(uint32_t)

static pci_device_t devices[MAX_PCI_DEVICE] = { 0 };
static uint16_t nb_device = 0;


uint16_t pci_read_8(pci_device_t* dev, uint8_t offset) {
    uint32_t address;
    uint32_t lbus = (uint32_t)dev->bus;
    uint32_t lslot = (uint32_t)dev->slot;
    uint32_t lfunc = (uint32_t)dev->func;
    uint32_t loffset = (uint32_t)offset;

    address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (loffset & 0xFC)) | (1 << 31);

    io_out32(PCI_COMMAND, address);

    return (uint8_t)((io_in32(PCI_DATA) >> ((offset & 3) * 8)) & 0xFF);
}

uint16_t pci_read_16(pci_device_t* dev, uint8_t offset) {
    uint32_t address;
    uint32_t lbus = (uint32_t)dev->bus;
    uint32_t lslot = (uint32_t)dev->slot;
    uint32_t lfunc = (uint32_t)dev->func;
    uint32_t loffset = (uint32_t)offset;

    address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (loffset & 0xFC)) | (1 << 31);

    io_out32(0xCF8, address);

    return (uint16_t)((io_in32(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
}

uint32_t pci_read_32(pci_device_t* dev, uint8_t offset) {
    uint32_t address;
    uint32_t lbus = (uint32_t)dev->bus;
    uint32_t lslot = (uint32_t)dev->slot;
    uint32_t lfunc = (uint32_t)dev->func;
    uint32_t loffset = (uint32_t)offset;

    address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (loffset & 0xFC)) | (1 << 31);

    io_out32(0xCF8, address);

    return io_in32(0xCFC);
}

void pci_write_8(pci_device_t* dev, uint8_t offset, uint8_t value) {
    uint32_t address;
    uint32_t lbus = (uint32_t)dev->bus;
    uint32_t lslot = (uint32_t)dev->slot;
    uint32_t lfunc = (uint32_t)dev->func;
    uint32_t loffset = (uint32_t)offset;

    address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (loffset & 0xFC)) | (1 << 31);

    io_out32(PCI_COMMAND, address);

    uint32_t oldval = io_in32(PCI_DATA);
    uint32_t mask = 0xFF;

    uint32_t bitoffset = (offset & 3) * 8;
    value = (value & mask) << bitoffset;
    oldval &= ~(mask << bitoffset);
    oldval |= value;

    io_out32(PCI_COMMAND, address);
    io_out32(PCI_DATA, oldval);
}

void pci_write_16(pci_device_t* dev, uint8_t offset, uint16_t value) {
    uint32_t address;
    uint32_t lbus = (uint32_t)dev->bus;
    uint32_t lslot = (uint32_t)dev->slot;
    uint32_t lfunc = (uint32_t)dev->func;
    uint32_t loffset = (uint32_t)offset;

    address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (loffset & 0xFC)) | (1 << 31);

    io_out32(PCI_COMMAND, address);

    uint32_t oldval = io_in32(PCI_DATA);
    uint32_t mask = 0xFFFF;

    uint32_t bitoffset = (offset & 3) * 8;
    value = (value & mask) << bitoffset;
    oldval &= ~(mask << bitoffset);
    oldval |= value;

    io_out32(PCI_COMMAND, address);
    io_out32(PCI_DATA, oldval);
}

void pci_write_32(pci_device_t* dev, uint8_t offset, uint32_t value) {
    uint32_t address;
    uint32_t lbus = (uint32_t)dev->bus;
    uint32_t lslot = (uint32_t)dev->slot;
    uint32_t lfunc = (uint32_t)dev->func;
    uint32_t loffset = (uint32_t)offset;

    address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (loffset & 0xFC)) | (1 << 31);

    io_out32(PCI_COMMAND, address);

    uint32_t oldval = io_in32(PCI_DATA);
    uint32_t mask = 0xFFFFFFFF;

    uint32_t bitoffset = (offset & 3) * 8;
    value = (value & mask) << bitoffset;
    oldval &= ~(mask << bitoffset);
    oldval |= value;

    io_out32(PCI_COMMAND, address);
    io_out32(PCI_DATA, oldval);
}

void get_bar(pci_device_t* dev) {
    for (uint8_t i = 0; i < 6; i++) {
        uint8_t reg = 0x4 + i;

        uint32_t base_low = pci_read_32(dev, REG(reg));
        if (base_low == 0)
            continue;
        pci_write_32(dev, REG(reg), ~0);
        uint32_t size_low = pci_read_32(dev, REG(reg));
        pci_write_32(dev, REG(reg), base_low);

        if (base_low & 1) {
            dev->bars[i].type = BAR_TYPE_IO;
            dev->bars[i].addr = base_low & ~0b11;
            dev->bars[i].size = (size_low & ~0b11) + 1;
        }
        else {
            int type = (base_low >> 1) & 3;
            uint32_t base_high = pci_read_32(dev, REG(reg) + 4);
            dev->bars[i].addr = base_low & ~0xF;
            dev->bars[i].size = ~(size_low & 0b1111) + 1;
            dev->bars[i].type = BAR_TYPE_MMIO;
            if (type == 2) {
                dev->bars[i].addr |= ((uint64_t)base_high << 32);
                i++;
            }
        }
    }
}

pci_device_t* pci_get_device_by_id(uint16_t id) {
    if (id >= nb_device) {
        return 0;
    }
    return &devices[id];
}

int PCI_get_device_by_filter(pci_device_t *filter, enum FILTER_MASK mask, int *id_out, int size){
    int c = 0;
    for (int i = 0; i < nb_device && c < size; i++) {
        if (mask & 1 && (devices[i].vendor_id != filter->vendor_id))
            continue;
        if (mask & 2 && (devices[i].device_id != filter->device_id))
            continue;
        if (mask & 4 && (devices[i].class_code != filter->class_code))
            continue;
        if (mask & 8 && (devices[i].subclass != filter->subclass))
            continue;
        if (mask & 16 && (devices[i].prog_if != filter->prog_if))
            continue;
        id_out[c] = i;
        c++;
    }
    return c;
}

void print_device(pci_device_t* dev) {
    logf("PCI Device: \n");
    logf("\tDevice ID: %x, Vendor ID: %x\n", dev->device_id, dev->vendor_id);
    logf("\tclass: %x, sub: %x, prog: %x\n", dev->class_code, dev->subclass, dev->prog_if);
    logf("\tint PIN: %d, int line: %d\n", dev->int_pin, dev->int_line);
    logf("\tPCI bars: \n");
    for (uint8_t i = 0; i < 6; i++) {
        if (dev->bars[i].addr != 0) {
            logf("\t\tbar %i: type %d, base %x\n", i, dev->bars[i].type, dev->bars[i].addr);
        }
    }
}

void pci_print_all_device() {
    for (uint16_t i = 0; i < nb_device; i++) {
        print_device(&devices[i]);
    }
}

void pci_scan_bus(uint8_t bus) {
    for (uint8_t slot = 0; slot < 32; slot++) {
        for (uint8_t func = 0; func < 8; func++) {
            pci_device_t* device = &devices[nb_device];

            device->bus = bus;
            device->slot = slot;
            device->func = func;

            uint32_t device_vendor = pci_read_32(device, REG(0));

            if (device_vendor == 0xFFFFFFFF) {
                continue;
            }

            uint32_t header_type = pci_read_8(device, REG(3) + 2);

            if ((header_type & 0x7F) != 0) {
                continue;
            }

            uint32_t class = pci_read_32(device, REG(2));
            uint16_t interrupts = pci_read_16(device, REG(0xF));

            device->device_id = (device_vendor >> 16) & 0xFFFF;
            device->vendor_id = (device_vendor) & 0xFFFF;
            device->class_code = (class >> 24) & 0xFF;
            device->subclass = (class >> 16) & 0xFF;
            device->prog_if = (class >> 8) & 0xFF;
            device->int_pin = (interrupts >> 8) & 0xFF;
            device->int_line = (interrupts) & 0xFF;
            get_bar(device);
            nb_device++;
        }
    }
}

void pci_fullscan() {
    nb_device = 0;
    pci_device_t dev = { 0 };
    uint16_t header_type = pci_read_8(&dev, 0xE);
    if (header_type & (1 << 7)) {
        // multiple buses
        pci_scan_bus(0);
    }
    else {
        for (uint8_t i = 0; i < 8; i++) {
            pci_scan_bus(i);
        }
    }
}
