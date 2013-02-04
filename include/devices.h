#ifndef DEVICES_H
#define DEVICES_H

#include <stdint.h>


void init_pit(void);

void pic_out8(uint16_t port, uint8_t value);
void pit_out8(uint16_t port, uint8_t value);

#endif
