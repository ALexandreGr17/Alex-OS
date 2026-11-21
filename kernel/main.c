#include "arch/interrupts/pic.h"
#include "logs/log.h"
#include <stdint.h>
#include <memory/physical/pmm.h>
#include <memory/virtual/vmm.h>

#include <boot/bootstrap.h>

#include <arch/interrupts/idt.h>
#include <pit/pit.h>
#include <arch/pci/pci.h>
#include <driver/ata.h>

void timer_handler(interrupt_frame_t* frame) {
    pic_send_eoi(0);
}


static void print_device(pci_device_t* dev) {
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

void kernel_main(bootstrap_info_t* bs_info) {
    clrscr();
    // puts("test\n\t");
    logf("Hello %s%c %i %x\n", "world", '!', 10, 10);

    logf("multiboot_struct: %x\n", bs_info->multiboot_struct);

    bootstrap_info_t* info = PHYS_TO_VIRT(bs_info);
    logf("pmm_map[0]: %x\n", ((uint64_t*)PHYS_TO_VIRT(info->pmm_map))[0]);
    idt_init();

    pit_init();
    init_pmm_higher(PHYS_TO_VIRT(info->pmm_map), info->pmm_map_size);
    init_vmm_higher();



    uint8_t* block = pmm_find_free_block(1);
    pmm_alloc_region((uint64_t)block, 4096);
    uint8_t* page = vmm_find_free_page();

    vmm_map_page(page, block, 1);

    page[0] = 0;
    page[0] = 1;

    logf("%x: %d\n", page, page[0]);

    sleep_ms(1000);
    logf("after 1 s\n");

    pci_fullscan();

    pci_print_all_device();

    int pci_id = 0;
    pci_device_t filter = {
        .class_code = 1,
        .subclass = 1,
        .prog_if = 0x80
    };
    PCI_get_device_by_filter(&filter, CLASS_CODE | SUBCLASS | PROG_IF, &pci_id, 1);

    pci_device_t* dev = pci_get_device_by_id(pci_id);
    print_device(dev);
    init_ata(dev);

    uint16_t buffer[256] = { 0 };
    logf("Before read: %x\n", buffer[0]);
    ata_read_28(0, buffer);
    logf("After read: %x\n", buffer[0]);


    buffer[0] = 0xDEAD;
    ata_write_48(0, buffer);
    ata_read_48(0, buffer);
    logf("After read_write: %x\n", buffer[0]);
    while(1);
}
