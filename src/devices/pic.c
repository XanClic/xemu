#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>

#define DEVICE_NAME "PIC"

#include "device.h"
#include "system_state.h"


extern volatile bool settle_threads;

extern volatile bool no_irqs_nao;

volatile int execute_irqs;
pthread_cond_t irq_update;

extern int call_int_vector;

extern pid_t vm_pid;


static bool icw4[2] = { false, false }, have_slave = true, auto_eoi[2] = { false, false }, in_irq[2] = { false, false };
static int irq_base[2] = { 0x08, 0x70 }, masked_irqs[2] = { 0, 0 }, in_init[2] = { 0, 0 };

void pic_out8(uint16_t port, uint8_t val)
{
    bool slave = !!(port & 0x80);

    if (slave && !have_slave)
    {
        dprintf("Oops, tried to access slave being not available!\n");
        return; //Einfach ignorieren
    }

    if (!(port & 1))
    {
        //Befehl
        if (val & 0x10)
        {
            if (in_init[slave])
                dprintf("%i: Warning: Was already in init mode, reiniting\n", slave);
            dprintf("%i: Starting initialisation.\n", slave);

            in_init[slave] = 1;

            icw4[slave] = !!(val & 0x01);

            if (!slave)
            {
                if (val & 0x02)
                {
                    dprintf("Hu? We don't have a slave... Funny, but... OK.\n");
                    have_slave = false;
                }
                else
                {
                    dprintf("OK, slave is available.\n");
                    have_slave = true;
                }
            }
        }
        else if (val == 0x20) // EOI
            in_irq[slave] = false;
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
}


void *irq_thread(void *arg)
{
    (void)arg;


    pthread_mutex_t cond_mtx = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_lock(&cond_mtx);

    while (!settle_threads)
    {
        while (!execute_irqs || !int_flag)
            pthread_cond_wait(&irq_update, &cond_mtx);


        bool may_interrupt;

        do
        {
            int status;
            waitpid(vm_pid, &status, WNOHANG);

            may_interrupt = !WIFSTOPPED(status);
        }
        while (no_irqs_nao || !may_interrupt);


        int first = ffs(execute_irqs) - 1;

        if (first == 2)
        {
            // dafuq? (IRQ 2 is slave)
            execute_irqs &= ~(1 << 2);
            continue;
        }

        // check slave before IRQs 3..7 (since he uses master IRQ 2)
        if ((first > 2) && (first < 8) && ffs(execute_irqs & 0xff00))
            first = ffs(execute_irqs & 0xff00) - 1;


        if (in_irq[0] || ((first >= 8) && in_irq[1]))
            continue; // still in IRQ

        if (((first < 8) && (masked_irqs[0] & (1 << first))) || ((first >= 8) && ((masked_irqs[0] & (1 << 2)) || (masked_irqs[1] & (1 << (first - 8))))))
            continue; // masked


        in_irq[0] = true;
        if (first >= 8)
            in_irq[1] = true;

        execute_irqs &= ~(1 << first);


        call_int_vector = irq_base[first >= 8] + (first & 7);

        kill(vm_pid, SIGUSR1);
    }

    return NULL;
}
