#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>


void segfault_handler(int, struct sigcontext);
extern void deinit_sdl(void);


void init_segfault_handler(void)
{
    stack_t stk;

    signal(SIGSEGV, (void (*)(int))&segfault_handler);
    stk.ss_sp = mmap((void *)0x7F000000, SIGSTKSZ, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
    stk.ss_size = SIGSTKSZ;
    stk.ss_flags = 0;
    sigaltstack(&stk, NULL);
}

void segfault_handler(int num, struct sigcontext ctx)
{
    uint8_t *instr;
    FILE *ffp;
    static enum
    {
        REAL_SEGFAULT = 0,
        CLI_SEGFAULT = 1
    } expected_segfault = REAL_SEGFAULT;
    static int old_values[20], old_len = 0;
    int may_return = 0;

    if (expected_segfault == CLI_SEGFAULT)
    {
        instr = (uint8_t *)(ctx.eip - (old_len - 1));
        for (int i = 0; i < old_len; i++)
            instr[i] = old_values[i];
        expected_segfault = REAL_SEGFAULT;
        return;
    }

    instr = (uint8_t *)ctx.eip;

    switch (instr[0])
    {
        case 0xEE:
            printf("out: 0x%02X -> 0x%04X\n", (unsigned int)(ctx.eax & 0xFF), (unsigned int)(ctx.edx & 0xFFFF));
            //out dx,al
            if (((ctx.edx & 0xFFFF) >= 0x3D0) && ((ctx.edx & 0xFFFF) <= 0x3DC))
            {
                //CGA - wird ignoriert
                may_return = 2;
                break;
            }
            else if ((((ctx.edx & 0xFFFF) >= 0x3F8) && ((ctx.edx & 0xFFFF) <= 0x3FF)) ||
                     (((ctx.edx & 0xFFFF) >= 0x2F8) && ((ctx.edx & 0xFFFF) <= 0x2FF)) ||
                     (((ctx.edx & 0xFFFF) >= 0x3E8) && ((ctx.edx & 0xFFFF) <= 0x3EF)) ||
                     (((ctx.edx & 0xFFFF) >= 0x2E8) && ((ctx.edx & 0xFFFF) <= 0x2EF)))
            {
                //COM - wird ignoriert
                may_return = 2;
                break;
            }
            break;
    }

    if (may_return)
    {
        for (int i = 0; i < may_return; i++)
            old_values[i] = instr[i];
        old_len = may_return;
        expected_segfault = CLI_SEGFAULT;
        may_return--;
        for (int i = 0; i < may_return; i++)
            instr[i] = 0x90; //NOP
        instr[may_return] = 0xFA; //CLI
        return;
    }

    printf("Unhandled segfault. All our base are belong to the OS.\n");
    printf("EIP: 0x%08X\n", (unsigned int)ctx.eip);
    printf("CR2: 0x%08X\n", (unsigned int)ctx.cr2);
    printf("Instructions: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n", instr[0], instr[1], instr[2], instr[3], instr[4], instr[5]);

    printf("Disassembling...\n");
    ffp = fopen("failed_code.bin", "w");
    if (ffp == NULL)
        printf("Failed: Cannot write binary data.\n");
    else
    {
        fwrite(instr, 1, 10, ffp);
        fclose(ffp);
        system("ndisasm -u failed_code.bin | head -n 1");
        remove("failed_code.bin");
    }

    deinit_sdl();

    exit(EXIT_FAILURE);
}
