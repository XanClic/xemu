#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include "elf.h"


extern void init_sdl(void);
extern uintptr_t load_elf(void *krnl);
extern void init_segfault_handler(void);


int main(int argc, char *argv[])
{
    FILE *kernel;
    void *krnl, *rp;
    int lof;
    uintptr_t entry;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: xemu <kernel>\n");
        return EXIT_FAILURE;
    }

    if (!access("/etc/dpkg/origins/ubuntu", F_OK))
    {
        fprintf(stderr, "Your system looks like Ubuntu.\n");
        fprintf(stderr, "So there's a problem. UBUNTU IS AS BROKEN AS GUIDO'S FACE.\n");
        fprintf(stderr, "If you're using Ubuntu, you may go on but I'm sure you'll get a segfault.\n");
        fprintf(stderr, "If not, try to continue.\n\n");
        fprintf(stderr, "Error explanation: We need to do a mmap on the first 128 MB. Ubuntu does\n");
        fprintf(stderr, "not allow that, whyever. While mapping that we get a segfault (which is a\n");
        fprintf(stderr, "really WRONG behaviour, it should return MAP_FAILED and xemu could catch\n");
        fprintf(stderr, "that but it doesn't). I don't know what's in there but it's just like that.\n\n");
        fprintf(stderr, "Do you want to continue? [y/n] ");
        fflush(stderr);
        if (getchar() != 'y')
        {
            fputc('\n', stderr);
            exit(EXIT_FAILURE);
        }
        fputc('\n', stderr);
    }

    kernel = fopen(argv[1], "r");
    if (kernel == NULL)
    {
        fprintf(stderr, "%s: %s\n", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }

    fseek(kernel, 0, SEEK_END);
    lof = ftell(kernel);

    krnl = mmap((void *)0x78000000, lof, PROT_READ, MAP_PRIVATE | MAP_FIXED, fileno(kernel), 0);
    if (krnl == MAP_FAILED)
    {
        fprintf(stderr, "Cannot load that kernel: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    fclose(kernel);

    //128 MB mappen (außer die ersten 64 kB, die erlaubt Linux uns nicht)
    rp = mmap((void *)0x00010000, 0x07FF0000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
    if ((rp == MAP_FAILED) || (rp != (void *)0x00010000))
    {
        fprintf(stderr, "Cannot allocate 128 MB.\n");
        return EXIT_FAILURE;
    }

    entry = load_elf(krnl);

    printf("Load done.\n");
    printf("In AD 2009 fun was beginning.\n");
    printf("Setting up us this program...\n");
    printf("(we have no chance to survive, making our segfault handler)\n");

    init_segfault_handler();

    printf("\n");

    init_sdl();

    asm volatile ("jmp *%%ebx" :: "b"(entry));

    return EXIT_SUCCESS;
}
