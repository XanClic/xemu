#include <errno.h>
#include <signal.h>
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
void ubuntu_gau(int);


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

        signal(SIGSEGV, &ubuntu_gau);
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

void ubuntu_gau(int sig)
{
    fprintf(stderr, "YEAH! I TOLD YOU! UBUNTU IS BROKEN! IT'S SO BROKEN I COULD CRY!\n");
    fprintf(stderr, "I hate Ubuntu users saying Ubuntu would be cool. You see, it isn't at all.\n");
    fprintf(stderr, "What to do now? You could use openSUSE. That's a good thing.\n");
    fprintf(stderr, "Or you build Linux on your own (Linux from scratch), that'll also work.\n");
    fprintf(stderr, "openSUSE: http://software.opensuse.org/112/en\n");
    fprintf(stderr, "Linux from scratch: http://www.linuxfromscratch.org/\n\n");
    fprintf(stderr, "So get rid off Ubuntu right NOW!\n");
    exit(EXIT_FAILURE);
}
