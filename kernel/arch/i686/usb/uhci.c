#include "uhci.h"
#include "arch/i686/io.h"
#include "arch/i686/isr.h"
#include "arch/i686/pic.h"
#include "arch/i686/pit.h"
#include "arch/i686/pci/pci.h"
#include "memory/memory.h"
#include "memory_management/memory_management.h"
#include "memory_management/virtual/virtual_memory_manager.h"
#include <stdint.h>
#include <stdio.h>

#define DEBUG_BIT(bit, reg) (((reg & bit) == 0) ? "OFF" : "ON")

void uhci_cmd_reg_debug(uint16_t reg) {
    printf("UHCI CMD REGISTER: \n");
    printf("\tRun: %s\n", DEBUG_BIT(UHCI_CMD_RUN, reg));
    printf("\tHost Controller Reset: %s\n", DEBUG_BIT(UHCI_CMD_HCR, reg));
    printf("\tGlobal Reset: %s\n", DEBUG_BIT(UHCI_CMD_GL_RES, reg));
    printf("\tGlobal Suspend: %s\n", DEBUG_BIT(UHCI_CMD_GL_SUS, reg));
    printf("\tGlobal Resume: %s\n", DEBUG_BIT(UHCI_CMD_GL_RESUME, reg));
    printf("\tSoftware Debug: %s\n", DEBUG_BIT(UHCI_CMD_SOFT_DBG, reg));
    printf("\tConfigure Flag: %s\n", DEBUG_BIT(UHCI_CMD_CONF_FLAG, reg));
    printf("\tMax Packet Size: %s\n", (((reg & UHCI_CMD_MAX_PACKET_SIZE) == 0) ? "32" : "64"));
}

void uhci_sts_reg_debug(uint16_t reg) {
    printf("UHCI STATUS REGISTER: \n");
    printf("\tInterrupt: %s\n", DEBUG_BIT(UHCI_STS_INT, reg));
    printf("\tError Interrupt: %s\n", DEBUG_BIT(UHCI_STS_ERR_INT, reg));
    printf("\tResume Detected: %s\n", DEBUG_BIT(UHCI_STS_RESUME, reg));
    printf("\tSystem error: %s\n", DEBUG_BIT(UHCI_STS_SYS_ERR, reg));
    printf("\tProcess error: %s\n", DEBUG_BIT(UHCI_STS_PROCESS_ERR, reg));
    printf("\tHalted: %s\n", DEBUG_BIT(UHCI_STS_HLT, reg));
}

void uhci_int_reg_debug(uint16_t reg) {
    printf("UHCI INTERUPT REGISTER: \n");
    printf("\tTimeout CRC: %s\n", DEBUG_BIT(UHCI_INT_TO_CRC, reg));
    printf("\tResume: %s\n", DEBUG_BIT(UHCI_INT_RESUME, reg));
    printf("\tComplete Transfert: %s\n", DEBUG_BIT(UHCI_INT_COMP_TRASN, reg));
    printf("\tShort Packet: %s\n", DEBUG_BIT(UHCI_INT_SHORT_PACKET, reg));
}

void uhci_port_reg_debug(uint16_t reg, uint16_t n) {
    printf("UHCI PORT %d REGISTER: \n", n);
    printf("\tConnection Status: %s\n", DEBUG_BIT(UHCI_PORT_CONN_STS, reg));
    printf("\tConnection Status Change: %s\n", DEBUG_BIT(UHCI_PORT_CONN_STS_CHG, reg));
    printf("\tDevice Enable: %s\n", DEBUG_BIT(UHCI_PORT_DEVICE_EN, reg));
    printf("\tPort Enable Change: %s\n", DEBUG_BIT(UHCI_PORT_PORT_EN_CHG, reg));
    printf("\tLine Status: %d\n", (reg & UHCI_PORT_LINE_STS) >> 4);
    printf("\tResume Detected: %s\n", DEBUG_BIT(UHCI_PORT_RESUME_DETEC, reg));
    printf("\tLow Speed Device: %s\n", ((UHCI_PORT_DEVICE_EN & reg) == 0) ? "Full Speed" : "Low Speed");
    printf("\tReset: %s\n", DEBUG_BIT(UHCI_PORT_RESET, reg));
    printf("\tSuspend: %s\n", DEBUG_BIT(UHCI_PORT_SUSPEND, reg));
}

void uhci_ctrl_debug(uhci_ctrl_t* ctrl) {
    uint16_t cmd = i686_inw(ctrl->port);
    uint16_t sts = i686_inw(ctrl->port + USBSTS);
    uint16_t inter = i686_inw(ctrl->port + USBINTR);
    uint16_t frame_number = i686_inw(ctrl->port + FRNUM);
    uint32_t frame_addr = i686_inl(ctrl->port + FRBASEADD);
    uint8_t sofmod = i686_inb(ctrl->port + SOFMOD);

    printf("UHCI controller debug:\n");
    printf("\tport: %x\n", ctrl->port);
    printf("\tframe_addr: %x\n", ctrl->frame_list);
    printf("\tframe_phys_addr: %x\n", frame_addr);
    printf("\tframe_phys_addr_check: %x\n", get_phys_addr((uint32_t)ctrl->frame_list));
    printf("\tframe_number: %d\n", frame_number);
    printf("\tsofmod: %x\n", sofmod);
    printf("\tnb port: %x\n", ctrl->nb_port);
    uhci_int_reg_debug(inter);
    uhci_sts_reg_debug(sts);
    uhci_cmd_reg_debug(cmd);
    for (uint16_t i = 0; i < ctrl->nb_port; i++) {
        uint16_t port = i686_inw(ctrl->port + PORTSCBASE + (i * 2));
        uhci_port_reg_debug(port, i);
    }
}

#define SET_ADDR(entry, addr) (entry = (entry & 0xF) | (addr & ~0xF))
#define ENRY_ENABLE_FRAME(entry) (*entry |= 1)
#define ENTRY_DISABLE_FRAME(entry) (*entry &= ~1)
#define SET_QH(entry) (*entry &= ~(1 << 1))
#define SET_TD(entry) (*entry |= (1 << 1))

#define TD_ND_SET_DEPTH(nd) (nd |= (1 << 2))
#define TD_ND_UNSET_DEPTH(nd) (nd &= ~(1 << 2))
#define TD_ND_SET_TERMINATE(nd) (nb |= 1)
#define TD_ND_UNSET_TERMINATE(nd) (nb &= ~1)


struct UHCI_td_s {
    uint32_t next_descriptor;
    uint32_t status;
    uint32_t packet_header;
    uint32_t buffer_address;
    uint8_t system_use[16];
};

struct UHCI_qh_s {
    uint32_t horizontal_ptr;
    uint32_t vertical_ptr;
};

void i686_UHCI_handler(Register* regs) {
    printf("yay\n");
}

int uhci_handle_connection(uhci_ctrl_t* ctrl, int port) {
    uint16_t legacy = i686_inw(ctrl->port + PORTSCBASE + port * 2);
    legacy |= UHCI_PORT_RESET;
    i686_outw(ctrl->port + PORTSCBASE + port * 2, legacy);

    sleep_ms(100);

    legacy = i686_inw(ctrl->port + PORTSCBASE + port * 2);
    legacy &= ~UHCI_PORT_RESET;
    i686_outw(ctrl->port + PORTSCBASE + port * 2, legacy);

    sleep_ms(50);

    legacy = i686_inw(ctrl->port + PORTSCBASE + port * 2);
    legacy |= UHCI_PORT_DEVICE_EN;
    i686_outw(ctrl->port + PORTSCBASE + port * 2, legacy);

    uint32_t timeout = 100;
    while (timeout--) {
        legacy = i686_inw(ctrl->port + PORTSCBASE + port * 2);
        if (legacy & UHCI_PORT_DEVICE_EN) {
            return 0;
        }
        sleep_ms(1);
    }
    
    legacy = i686_inw(ctrl->port + PORTSCBASE + port * 2);
    legacy &= ~UHCI_PORT_DEVICE_EN;
    i686_outw(ctrl->port + PORTSCBASE + port * 2, legacy);

    return -1;
}

void uhci_check_port(uhci_ctrl_t* ctrl) {
    for (int i = 0; i < ctrl->nb_port; i++) {
        uint16_t port = i686_inw(ctrl->port + PORTSCBASE + 2 * i);

        if (port & UHCI_PORT_CONN_STS_CHG) {
            port &= ~UHCI_PORT_CONN_STS_CHG;
            i686_outw(ctrl->port + PORTSCBASE + port * 2, port);

            if (port & UHCI_PORT_CONN_STS) {
                if (uhci_handle_connection(ctrl, i) < 0) {
                    continue;
                }
            }
        }
    }

    uhci_ctrl_debug(ctrl);
}

uhci_ctrl_t* uhci_init(PCI_device_t* dev) {

    uint16_t legacy = PCI_controller_read(dev, 0xC0, WORD);
    PCI_controller_write(dev, 0xC0, legacy | 0x2000, WORD);

    if ((PCI_controller_read(dev, 0xC0, WORD) & 0x2000) == 0) {
        printf("Error: Legacy USB still active\n");
        return NULL;
    }

    legacy = PCI_controller_read(dev, 0x4, WORD);
    PCI_controller_write(dev, 0x4, legacy | (1 << 2), WORD);

    if ((PCI_controller_read(dev, 0x4, WORD) & (1 << 2)) == 0) {
        printf("Error: Bus Mastering is disable\n");
        return NULL;
    }

    uhci_ctrl_t* ctrl = calloc(1, sizeof(uhci_ctrl_t));
    ctrl->dev = dev;
    ctrl->port = dev->bars[4].addr;

    uint16_t cmd = i686_inw(ctrl->port);
    cmd |= UHCI_CMD_HCR | UHCI_CMD_GL_RES;
    i686_outw(ctrl->port, cmd);

    sleep_ms(10);
    cmd = i686_inw(ctrl->port);
    cmd &= ~(UHCI_CMD_GL_RES);
    i686_outw(ctrl->port, cmd);

    while (cmd & UHCI_CMD_HCR) {
        i686_iowait();
        cmd = i686_inw(ctrl->port);
    }

    ctrl->frame_list = mmap(NULL, 1);
    for (uint32_t i = 0; i < 1024; i++) {
        ctrl->frame_list[i] = 1;
    }
    uint32_t frame_list_addr = (uint32_t)get_phys_addr((uint32_t)ctrl->frame_list);

    i686_outl(ctrl->port + FRBASEADD, frame_list_addr);

    legacy = i686_inw(ctrl->port + USBINTR);
    legacy |= UHCI_INT_COMP_TRASN | UHCI_INT_SHORT_PACKET;
    i686_outw(ctrl->port + USBINTR, legacy);


    for (int i = 0; i <= 2; i+=2) {
        legacy = i686_inw(ctrl->port + PORTSCBASE + i);
        if (legacy == 0xFFF || (legacy & (1 << 7)) == 0) {
            break;
        }
        ctrl->nb_port++;
    }

    legacy = i686_inw(ctrl->port);
    legacy |= UHCI_CMD_RUN;
    i686_outw(ctrl->port, legacy);

    i686_ISR_Registerhandler(ctrl->dev->int_line + 0x20, i686_UHCI_handler);

    uhci_check_port(ctrl);
}
