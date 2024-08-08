#ifndef PIC_H
#define PIC_H

#include <stdint.h>

void i686_PIC_Configure(uint8_t offsetPic1, uint8_t offsetPic2);
void i686_PIC_Disable();
void i686_PIC_Mask(int irq);
uint16_t i686_PIC_ReadIRQRequestRegister();
uint16_t i686_PIC_ReadInServiceRegister();
void i686_PIC_SendEOI(int irq);

#endif
