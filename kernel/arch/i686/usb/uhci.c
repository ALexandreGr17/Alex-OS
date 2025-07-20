#include "uhci.h"
#include "arch/i686/io.h"
#include "arch/i686/irq.h"
#include "arch/i686/pic.h"
#include "arch/i686/pit.h"
#include "arch/i686/pci/pci.h"
#include "memory/memory.h"
#include "memory/dma_allocator.h"
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

#define MAX_UHCI_CTRL 5

static uhci_ctrl_t* ctrl_list[MAX_UHCI_CTRL];
static uint32_t nb_ctrl = 0;
static uint8_t* data = NULL;

void handle_device_descriptor(uint8_t* data) {
    uint16_t vendor_id  = data[8] | (data[9] << 8);
    uint16_t product_id = data[10] | (data[11] << 8);
    uint8_t device_class = data[4];
    uint8_t max_packet_size = data[7];
    uint16_t usb_version = data[2] | (data[3] << 8);
    uint16_t device_release = data[12] | (data[13] << 8);
    uint8_t manufacturer_str_idx = data[14];
    uint8_t product_str_idx = data[15];
    uint8_t serial_str_idx = data[16];
    uint8_t num_configurations = data[17];


    printf("USB Device Descriptor:\n");
    printf("  Vendor ID: 0x%x\n", vendor_id);
    printf("  Product ID: 0x%x\n", product_id);
    printf("  Class: 0x%x\n", device_class);
    printf("  Max Packet Size: %d bytes\n", max_packet_size);
    printf("  USB Version: %x.%02x\n", (usb_version >> 8), (usb_version & 0xFF));
    printf("  Device Release: %x.%02x\n", (device_release >> 8), (device_release & 0xFF));
    printf("  Manufacturer String Index: %d\n", manufacturer_str_idx);
    printf("  Product String Index: %d\n", product_str_idx);
    printf("  Serial String Index: %d\n", serial_str_idx);
    printf("  Number of Configurations: %d\n", num_configurations);
}

#define DEBUG_ADDR(addr) (printf(#addr " = %x, " #addr "_phys = %x\n", addr, (uint32_t)get_phys_addr((uint32_t)addr)))
void i686_UHCI_handler(Register* regs) {

    i686_PIC_SendEOI(11);
    for (uint32_t i = 0; i < nb_ctrl; i++) {
        uint16_t usbsts = i686_inw(ctrl_list[i]->port + USBSTS);
        if (usbsts & UHCI_STS_INT) {
            i686_outw(ctrl_list[i]->port + USBSTS, usbsts);
            uhci_ctrl_debug(ctrl_list[i]);
            handle_device_descriptor(data);
        }
    }

}

enum USB_PACKET_TYPE {
    SETUP = 0x2D,
    IN = 0x69,
    OUT = 0xE1
};

typedef struct usb_packet_s {
    struct usb_packet_s* next;
    enum USB_PACKET_TYPE type;
    uint8_t* data;
    uint32_t data_len;
    uint32_t status;
} usb_packet_t;

typedef struct __attribute__((packed)) usb_setup_packet_s {
    uint8_t request_type;
    uint8_t request;
    uint16_t value;
    uint16_t index;
    uint16_t length;
} usb_setup_packet_t;

struct UHCI_td_s* construct_td(usb_packet_t* packet, uint8_t toggle) {
    struct UHCI_td_s* td = dma_alloc(sizeof(struct UHCI_td_s), 0x10);
    td->status = packet->status;
    td->packet_header = packet->type | (toggle << 19) | (packet->data_len > 0 ? (packet->data_len - 1) << 21  : 0);
    td->buffer_address = (uint32_t)get_phys_addr((uint32_t)packet->data);
    if (packet->next == NULL) {
        td->next_descriptor = 1;
        return td;
    }
    td->next_descriptor = (uint32_t)get_phys_addr((uint32_t)construct_td(packet->next, !toggle));
    return td;
}

void send_packets(uhci_ctrl_t* ctrl, usb_packet_t* packet) {
    if (packet == NULL || ctrl == NULL){
        return;
    }
    struct UHCI_td_s* td = construct_td(packet, 0);
    struct UHCI_qh_s* qh = dma_alloc(sizeof(struct UHCI_qh_s), 0x10);
    qh->vertical_ptr = (uint32_t)get_phys_addr((uint32_t)td);
    qh->horizontal_ptr = 1;
    
    uint32_t frame_entry = (uint32_t)get_phys_addr((uint32_t)qh) | (1 << 1);
    ctrl->frame_list[0] = frame_entry;
};

void send_get_descriptor_cmd(uhci_ctrl_t* ctrl, int port) {

    usb_setup_packet_t* setup_packet_data = dma_alloc(sizeof(usb_setup_packet_t), 1);
    setup_packet_data->request_type = 0x80;
    setup_packet_data->request = 0x06;
    setup_packet_data->value = 0x0100;
    setup_packet_data->index = 0;
    setup_packet_data->length = 0x12;

    usb_packet_t* packet_out = malloc(sizeof(usb_packet_t));
    packet_out->next = 0;
    packet_out->status = (1 << 23) | (1 << 24);
    packet_out->type = OUT;
    packet_out->data_len = 0;
    packet_out->data = NULL;

    data = dma_alloc(18, 0x10);

    usb_packet_t* packet_in = malloc(sizeof(usb_packet_t));
    packet_in->next = packet_out;
    packet_in->type = IN;
    packet_in->status = (1 << 23);
    packet_in->data = data;
    packet_in->data_len = 18;

    usb_packet_t* setup_packet = malloc(sizeof(usb_packet_t));
    setup_packet->next = packet_in;
    setup_packet->data = (uint8_t*)setup_packet_data;
    setup_packet->data_len = sizeof(usb_setup_packet_t);
    setup_packet->type = SETUP;
    setup_packet->status = (1 << 23);

    send_packets(ctrl, setup_packet);

}

void send_set_address_cmd(uhci_ctrl_t* ctrl) {
    usb_setup_packet_t* setup_packet_data = malloc(sizeof(usb_setup_packet_t));
    setup_packet_data->request_type = 0;
    setup_packet_data->request_type = 5;
    setup_packet_data->value = 1;
    setup_packet_data->index = 0;
    setup_packet_data->length = 0;

    usb_packet_t* packet_setup = malloc(sizeof(usb_packet_t));
    packet_setup->next = NULL;
    packet_setup->type = SETUP;
    packet_setup->data = (uint8_t*)setup_packet_data;
    packet_setup->data_len = sizeof(usb_setup_packet_t);
    packet_setup->status = (1 << 23);
    send_packets(ctrl, packet_setup);
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
    send_get_descriptor_cmd(ctrl, 0);
    //send_set_address_cmd(ctrl);
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

    i686_IRQ_RegisterHandler(ctrl->dev->int_line, i686_UHCI_handler);

    ctrl_list[nb_ctrl] = ctrl;
    nb_ctrl++;
    uhci_check_port(ctrl);
}
