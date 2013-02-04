#include <stdint.h>
#include <stdio.h>

#include "devices.h"
#include "pio.h"




void out8(uint16_t port, uint8_t value)
{
    // TODO: Schöner machen
    if ((port & 0xFF7E) == 0x20)
        pic_out8(port, value);
    else if ((port & 0xFFFC) == 0x40)
        pit_out8(port, value);
    else
        fprintf(stderr, "[pio] unhandled out8 of 0x%02x on port 0x%04x\n", value, port);
}
