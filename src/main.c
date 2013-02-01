#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include "elf.h"
#include "execute.h"
#include "forker.h"


static void help(void)
{
    fprintf(stderr, "Usage: xemu --kernel/-k <mboot elf> [--module/-m <mod>]*\n");
}


int main(int argc, char *argv[])
{
    const char *kname = NULL;
    int modules[32]; // FIXME
    int mod_count = 0;

    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
        {
            help();
            return EXIT_SUCCESS;
        }
        else if (!strcmp(argv[i], "--kernel") || !strcmp(argv[i], "-k"))
        {
            if (i == argc - 1)
            {
                help();
                return EXIT_FAILURE;
            }
            kname = argv[++i];
        }
        else if (!strcmp(argv[i], "--module") || !strcmp(argv[i], "-m"))
        {
            if (i == argc - 1)
            {
                help();
                return EXIT_FAILURE;
            }
            modules[mod_count++] = ++i;
        }
        else
        {
            fprintf(stderr, "Unrecognized argument “%s”.\n", argv[i]);
            help();
            return EXIT_FAILURE;
        }
    }

    if (kname == NULL)
    {
        help();
        return EXIT_FAILURE;
    }


    FILE *fp = fopen(kname, "rb");

    if (fp == NULL)
    {
        perror(kname);
        return EXIT_FAILURE;
    }

    fseek(fp, 0, SEEK_END);
    size_t fsz = ftell(fp);
    rewind(fp);

    void *kernel = malloc(fsz);
    fread(kernel, 1, fsz, fp);

    fclose(fp);


    int shmfd = shm_open("/xemu_phys_ram", O_RDWR | O_CREAT, 0777);
    dup2(shmfd, 15);
    close(shmfd);

    ftruncate(15, 0x08000000);

    mmap((void *)0x100000000, 0x08000000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, 15, 0);


    uintptr_t kernel_entry = load_elf(kernel);

    if (!kernel_entry)
    {
        fprintf(stderr, "%s: Not a valid ELF.\n", kname);
        return EXIT_FAILURE;
    }


    fork_vm(kernel_entry);


    execute_vm();


    return EXIT_SUCCESS;
}
