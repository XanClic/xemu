#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include "devices.h"
#include "elf.h"
#include "execute.h"
#include "forker.h"
#include "memory.h"
#include "multiboot.h"
#include "paging.h"


extern void init_sdl(void);

extern void *irq_thread(void *arg);


volatile bool settle_threads;

extern pthread_cond_t irq_update;


static void help(void)
{
    fprintf(stderr, "Usage: xemu --kernel/-k <mboot elf> [--module/-m <mod>]*\n");
}


void unlink_shm(void);


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

    ftruncate(15, MEMSZ);

    mmap(adr_p2h(0), MEMSZ, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, 15, 0);


    full_paging_update();


    uintptr_t kernel_entry = load_elf(kernel);

    if (!kernel_entry)
    {
        fprintf(stderr, "%s: Not a valid ELF.\n", kname);
        return EXIT_FAILURE;
    }


    // FIXME: Lass ma Multibootheader checken
    struct multiboot_info *mbi = adr_g2h(0x00010000);
    mbi->mi_flags = (1 << 6) | (1 << 3);
    mbi->mem_lower = 576;
    mbi->mem_upper = MEMSZ - 10485876;
    mbi->cmdline = 0x00010800;
    mbi->mods_count = mod_count;
    mbi->mods_addr = 0x00012000;
    mbi->mmap_length = 96;
    mbi->mmap_addr = 0x00011000;


    strcpy(adr_g2h(mbi->cmdline), kname);


    uintptr_t base = 0x00013000;

    struct multiboot_module *mods = adr_g2h(mbi->mods_addr);
    for (int i = 0; i < mod_count; i++)
    {
        FILE *fp = fopen(argv[modules[i]], "rb");
        if (fp == NULL)
        {
            fprintf(stderr, "%s: %s\n", argv[modules[i]], strerror(errno));
            return EXIT_FAILURE;
        }

        fseek(fp, 0, SEEK_END);
        size_t sz = ftell(fp);
        rewind(fp);

        fread(adr_g2h(base), 1, sz, fp);

        fclose(fp);

        mods[i].mod_start = base;
        mods[i].mod_end   = base + sz;
        mods[i].string    = base + sz;

        size_t len = strlen(argv[modules[i]]) + 1;
        memcpy(adr_g2h(mods[i].string), argv[modules[i]], len);

        // Sieht schöner aus. GRUB und so scheinen das auch zu machen.
        base = (base + sz + len + 4095) & ~0xfff;
    }


    struct memory_map *mm = adr_g2h(mbi->mmap_addr);
    mm[0].size   = 20;
    mm[0].base   = 0x00000000;
    mm[0].length = 0x00001000;
    mm[0].type   = 0;
    mm[1].size   = 20;
    mm[1].base   = 0x00001000;
    mm[1].length = 0x0009f000;
    mm[1].type   = 1;
    mm[2].size   = 20;
    mm[2].base   = 0x000a0000;
    mm[2].length = 0x00060000;
    mm[2].type   = 0;
    mm[3].size   = 20;
    mm[3].base   = 0x00100000;
    mm[3].length = MEMSZ - 1048576;
    mm[3].type   = 1;


    atexit(&unlink_shm);


    init_sdl();


    fork_vm(kernel_entry);


    pthread_t irq_thread_var;
    pthread_create(&irq_thread_var, NULL, &irq_thread, NULL);

    init_pit();

    execute_vm();

    settle_threads = true;
    pthread_cond_broadcast(&irq_update);

    pthread_join(irq_thread_var, NULL);


    return EXIT_SUCCESS;
}


void unlink_shm(void)
{
    shm_unlink("/xemu_phys_ram");
    shm_unlink("/xemu_vm_comm");
}
