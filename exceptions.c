#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "exceptions.h"


extern void deinit_sdl(void);


void exception(int num, struct sigcontext *ctx)
{
    uint8_t *instr;
    FILE *ffp;

    instr = (uint8_t *)ctx->eip;

    printf("Cannot handle exceptions yet.\n");

    printf("Triple Fault. All your base are belong to us.\n");
    printf("EIP: 0x%08X\n", (unsigned int)ctx->eip);
    printf("CR2: 0x%08X\n", (unsigned int)ctx->cr2);
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
