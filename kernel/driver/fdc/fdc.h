#ifndef FDC_H
#define FDC_H

#include <stdint.h>

void fdc_init();
void floppy_read(uint32_t lba, uint8_t drive, uint8_t* data_out);

#endif
