#include <stdio.h>

#define DEVICE_NAME "PIC"

#include "device.h"

static int irq_base[2] = {0x08, 0x70}, in_init[2] = {0, 0}, icw4[2] = {0, 0}, have_slave = 1;
static int masked_irqs[2] = {0, 0}, auto_eoi[2] = {0, 0};

int pic_outb(int port, int val)
{
    int slave = !!(port & 0x80);

    if (slave && !have_slave)
    {
        dprintf("Oops, tried to access slave being not available!\n");
        return 1; //Einfach ignorieren
    }

    if (!(port & 1))
    {
        //Befehl
        if (in_init[slave])
            dprintf("%i: Warning: Was already in init mode, reiniting\n", slave);
        if (val & 0x10)
        {
            dprintf("%i: Starting initialisation.\n", slave);
            in_init[slave] = 1;
        }
        if (val & 0x01)
            icw4[slave] = 1;
        else
            icw4[slave] = 0;
        if (!slave)
        {
            if (val & 0x02)
            {
                dprintf("Hu? We don't have a slave... Funny, but... OK.\n");
                have_slave = 0;
            }
            else
            {
                dprintf("OK, slave is available.\n");
                have_slave = 1;
            }
        }
    }
    else
    {
        switch (in_init[slave])
        {
            case 0:
                //IRQs maskieren
                masked_irqs[slave] = val;
                dprintf("%i: Remasked IRQs: 0x%02X\n", slave, masked_irqs[slave]);
                break;
            case 1:
                //ICW2
                if (val & 0x07)
                    dprintf("%i: Base %i is not multiple of 8.\n", slave, val);
                irq_base[slave] = val & 0xF8;
                dprintf("%i: New IRQ base: %i\n", slave, val & 0xF8);
                if (have_slave)
                    in_init[slave] = 2;
                else if (icw4[slave])
                    in_init[slave] = 3;
                else
                {
                    in_init[slave] = 0;
                    dprintf("%i: Init done.\n", slave);
                }
                break;
            case 2:
                //ICW3
                if (!slave && (val != 0x04))
                    dprintf("%i: Slaves should be at 0x%02X, but there's one being at 0x04.\n", slave, val);
                else if (slave && (val != 2))
                    dprintf("%i: I should be at %i, but I'm at 2.\n", slave, val);
                if (icw4[slave])
                    in_init[slave] = 3;
                else
                {
                    in_init[slave] = 0;
                    dprintf("%i: Init done.\n", slave);
                }
                break;
            case 3:
                //ICW4
                if (!(val & 0x01))
                    dprintf("%i: Hu, not for PCs? Ignoring that shit.\n", slave);
                auto_eoi[slave] = !!(val & 0x02);
                in_init[slave] = 0;
                dprintf("%i: Init done.\n", slave);
                break;
        }
    }

    return 1;
}
