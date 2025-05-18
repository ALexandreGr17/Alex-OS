#ifndef USB_UHCI_H
#define USB_UHCI_H

#include <arch/i686/pci/pci.h>
#include <stdint.h>

typedef struct uhci_controller_s {
    PCI_device_t* dev;
    uint16_t port;
    uint32_t* frame_list;
    uint32_t nb_port;
} uhci_ctrl_t;

enum UHCI_IO_REG_OFFSET {
    USBCMD      = 0,
    USBSTS      = 0x2,
    USBINTR     = 0x4,
    FRNUM       = 0x6,
    FRBASEADD   = 0x8,
    SOFMOD      = 0xC,
    PORTSCBASE  = 0x10,
};

enum UHCI_CMD_REG_FLAG {
    UHCI_CMD_RUN                = 1,
    UHCI_CMD_HCR                = (1 << 1),
    UHCI_CMD_GL_RES             = (1 << 2),
    UHCI_CMD_GL_SUS             = (1 << 3),
    UHCI_CMD_GL_RESUME          = (1 << 4),
    UHCI_CMD_SOFT_DBG           = (1 << 5),
    UHCI_CMD_CONF_FLAG          = (1 << 6),
    UHCI_CMD_MAX_PACKET_SIZE    = (1 << 7)
};

enum UHCI_STATUS_REG_FLAG {
    UHCI_STS_INT            = 1,
    UHCI_STS_ERR_INT        = (1 << 1),
    UHCI_STS_RESUME         = (1 << 2),
    UHCI_STS_SYS_ERR        = (1 << 3),
    UHCI_STS_PROCESS_ERR    = (1 << 4),
    UHCI_STS_HLT            = (1 << 5)
};

enum UHCI_INT_REG_FLAG {
    UHCI_INT_TO_CRC         = 1,
    UHCI_INT_RESUME         = (1 << 1),
    UHCI_INT_COMP_TRASN     = (1 << 2),
    UHCI_INT_SHORT_PACKET   = (1 << 3),
};

enum UHCI_PORT_SC_REG {
    UHCI_PORT_CONN_STS      = 1,
    UHCI_PORT_CONN_STS_CHG  = (1 << 1),
    UHCI_PORT_DEVICE_EN     = (1 << 2),
    UHCI_PORT_PORT_EN_CHG   = (1 << 3),
    UHCI_PORT_LINE_STS      = (1 << 4) | (1 << 5),
    UHCI_PORT_RESUME_DETEC  = (1 << 6),
    UHCI_PORT_LOW_SPEED     = (1 << 8),
    UHCI_PORT_RESET         = (1 << 9),
    UHCI_PORT_SUSPEND       = (1 << 12)
};

#define READ_REG(reg, info) (reg & info != 0)


uhci_ctrl_t* uhci_init(PCI_device_t* dev);

 #endif
