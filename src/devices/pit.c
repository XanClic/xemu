#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>

#include "devices.h"


#define usecs(counter) ((counter) ? (1000000ULL * (counter) / 1193182) : (1000000ULL * 0x10000 / 1193182))


extern pthread_cond_t irq_update;
extern volatile int execute_irqs;


static int channel_mode[3] = { 0x02, 0x00, 0x00 };
static int counters[3] = { 0xffff, 0, 0 };
static bool got_lsb[3];

static void alarm_handler(int sig);


static void setup_timer(suseconds_t initial, suseconds_t interval)
{
    setitimer(ITIMER_REAL, &(struct itimerval){ .it_value = { .tv_sec = 0, .tv_usec = initial }, .it_interval = { .tv_sec = 0, .tv_usec = interval } }, NULL);
}


void init_pit(void)
{
    sigaction(SIGALRM, &(struct sigaction){ .sa_handler = &alarm_handler }, NULL);

    setup_timer(usecs(0), usecs(0));
}


void pit_out8(uint16_t port, uint8_t value)
{
    if ((port == 0x41) || (port == 0x42))
        return; // TODO

    bool update = false;


    if (port == 0x40)
    {
        assert((channel_mode[0] & (3 << 4)));

        switch ((channel_mode[0] >> 4) & 3)
        {
            case 1:
                counters[0] = (counters[0] & 0xff00) | value;
                update = true;
                break;
            case 2:
                counters[0] = (counters[0] & 0x00ff) | (value << 8);
                update = true;
                break;
            case 3:
                if (got_lsb[0])
                    counters[0] = (counters[0] & 0x00ff) | (value << 8);
                else
                    counters[0] = (counters[0] & 0xff00) | value;
                update = got_lsb[0];
                got_lsb[0] = !got_lsb[0];
        }
    }
    else
    {
        int channel = value >> 6;

        if (channel == 3)
            return;

        channel_mode[channel] = value & 0x3f;

        update = !(value >> 6);
    }


    if (update)
    {
        int mode = (channel_mode[0] >> 1) & 7;

        switch (mode)
        {
            case 0:
            case 4:
                setup_timer(usecs(0), 0);
                break;
            case 1:
            case 5:
                setup_timer(0, 0);
                break;
            case 2:
            case 3:
                setup_timer(usecs(counters[0]), usecs(counters[0]));
                break;
        }
    }
}


static void alarm_handler(int sig)
{
    (void)sig;

    execute_irqs |= (1 << 0);
    pthread_cond_broadcast(&irq_update);
}
