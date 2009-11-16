#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "elf.h"


void segfault_handler(int, struct sigcontext);


int main(int argc, char *argv[])
{
    FILE *kernel;
    void *krnl, *base, *rp;
    int lof, size;
    stack_t stk;

    if (argc != 2)
    {
        printf("Usage: xemu <kernel>\n");
        return EXIT_FAILURE;
    }

    kernel = fopen(argv[1], "r");
    if (kernel == NULL)
    {
        printf("%s: %s\n", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }

    fseek(kernel, 0, SEEK_END);
    lof = ftell(kernel);

    krnl = mmap((void *)0x78000000, lof, PROT_READ, MAP_PRIVATE | MAP_FIXED, fileno(kernel), 0);
    if (krnl == MAP_FAILED)
    {
        printf("Could not load that kernel: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    fclose(kernel);

    printf("Kernel mapped to 0x%08X, interpreting...\n", (unsigned int)krnl);

    Elf32_Ehdr *elf_hdr = krnl;

    if ((elf_hdr->e_ident[0] != 0x7F) ||
        (elf_hdr->e_ident[1] !=  'E') ||
        (elf_hdr->e_ident[2] !=  'L') ||
        (elf_hdr->e_ident[3] !=  'F'))
    {
        printf("That's no ELF.\n");
        return EXIT_FAILURE;
    }

    if ((elf_hdr->e_ident[EI_CLASS] != ELFCLASS32)  ||
        (elf_hdr->e_ident[EI_DATA]  != ELFDATA2LSB) ||
        (elf_hdr->e_machine         != EM_386)      ||
        (elf_hdr->e_type            != ET_EXEC))
    {
        printf("i386 LSB executable required.\n");
        return EXIT_FAILURE;
    }

    printf("Valid format, loading...\n");

    //128 MB mappen (außer die ersten 64 kB, die erlaubt Linux uns nicht)
    rp = mmap((void *)0x00010000, 0x07FF0000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
    if ((rp == MAP_FAILED) || (rp != (void *)0x00010000))
    {
        printf("Could not allocate 128 MB.\n");
        return EXIT_FAILURE;
    }

    Elf32_Phdr *prg_hdr = (Elf32_Phdr *)((uintptr_t)krnl + elf_hdr->e_phoff);
    for (int i = 0; i < elf_hdr->e_phnum; i++)
    {
        //Laden sollte man das schon können.
        if (prg_hdr[i].p_type != PT_LOAD)
            continue;
        printf("Loading %i bytes (memsize) to 0x%08X\n", (int)prg_hdr[i].p_memsz, (unsigned int)prg_hdr[i].p_vaddr);
        base = (void *)(prg_hdr[i].p_vaddr & 0xFFFFF000);
        size = prg_hdr[i].p_memsz + (prg_hdr[i].p_vaddr & 0x00000FFF);
        printf(" -> padded to %i@0x%08X\n", size, (unsigned int)base);
        memset((void *)prg_hdr[i].p_vaddr, 0, prg_hdr[i].p_memsz);
        if (!prg_hdr[i].p_filesz)
            continue;
        memcpy((void *)prg_hdr[i].p_vaddr, (const void *)((uintptr_t)krnl + prg_hdr[i].p_offset), prg_hdr[i].p_filesz);
    }

    printf("Load done.\n");

    printf("In AD 2009 fun was beginning.\n");

    printf("Setting up us this program...\n");

    printf("(we have no chance to survive, making our segfault handler)\n");

    signal(SIGSEGV, (void (*)(int))&segfault_handler);
    stk.ss_sp = mmap((void *)0x7F000000, SIGSTKSZ, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
    stk.ss_size = SIGSTKSZ;
    stk.ss_flags = 0;
    sigaltstack(&stk, NULL);

    printf("\n");

    asm volatile ("jmp *%%ebx" :: "b"(elf_hdr->e_entry));

    return EXIT_SUCCESS;
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
        printf("Failed: Could not write binary data.\n");
    else
    {
        fwrite(instr, 1, 10, ffp);
        fclose(ffp);
        system("ndisasm -u failed_code.bin | head -n 1");
        remove("failed_code.bin");
    }

    printf("Dump of 0xB8000:\n");
    char *base = (char *)0xB8000;
    putchar('+');
    for (int x = 1; x < 81; x++)
        putchar('-');
    putchar('+');
    putchar('\n');
    for (int y = 0; y < 25; y++)
    {
        putchar('|');
        for (int x = 0; x < 80; x++)
        {
            if (*base < 0x20)
                putchar(' ');
            else
                putchar(*base);
            base += 2;
        }
        putchar('|');
        putchar('\n');
    }
    putchar('+');
    for (int x = 1; x < 81; x++)
        putchar('-');
    putchar('+');
    putchar('\n');

    exit(EXIT_FAILURE);
}
