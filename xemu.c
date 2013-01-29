#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include "elf.h"
#include "multiboot.h"


extern void init_sdl(void);
extern uintptr_t load_elf(void *krnl);
extern void init_segfault_handler(void);


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


    // TODO: Ist das mal gefixt?
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
            return EXIT_FAILURE;
        }
        fputc('\n', stderr);
    }

    FILE *kernel = fopen(kname, "rb");
    if (kernel == NULL)
    {
        fprintf(stderr, "%s: %s\n", kname, strerror(errno));
        return EXIT_FAILURE;
    }

    fseek(kernel, 0, SEEK_END);
    size_t lof = ftell(kernel);

    void *krnl = mmap((void *)0x78000000, lof, PROT_READ, MAP_PRIVATE | MAP_FIXED, fileno(kernel), 0);
    if (krnl == MAP_FAILED)
    {
        fprintf(stderr, "Cannot load that kernel: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    fclose(kernel);

    //128 MB mappen (außer die ersten 64 kB, die erlaubt Linux uns nicht)
    void *rp = mmap((void *)0x00010000, 0x07FF0000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
    if ((rp == MAP_FAILED) || (rp != (void *)0x00010000))
    {
        fprintf(stderr, "Cannot allocate 128 MB.\n");
        return EXIT_FAILURE;
    }


    // FIXME: Lass ma Multibootheader checken
    struct multiboot_info *mbi = (struct multiboot_info *)0x00010000;
    mbi->mi_flags = (1 << 6) | (1 << 3);
    mbi->mem_lower = 576;
    mbi->mem_upper = 127 * 1024;
    mbi->cmdline = 0x00010800;
    mbi->mods_count = mod_count;
    mbi->mods_addr = 0x00012000;
    mbi->mmap_length = 96;
    mbi->mmap_addr = 0x00011000;

    strcpy((char *)mbi->cmdline, kname);

    uintptr_t base = 0x00013000;

    struct multiboot_module *mods = (struct multiboot_module *)mbi->mods_addr;
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

        fread((void *)base, 1, sz, fp);

        fclose(fp);

        mods[i].mod_start = base;
        mods[i].mod_end   = base + sz;
        mods[i].string    = base + sz;

        size_t len = strlen(argv[modules[i]]) + 1;
        memcpy((void *)mods[i].string, argv[modules[i]], len);

        // Sieht schöner aus. GRUB und so scheinen das auch zu machen.
        base = (base + sz + len + 4095) & ~0xFFF;
    }

    struct memory_map *mm = (struct memory_map *)mbi->mmap_addr;
    mm[0].size   = 20;
    mm[0].base   = 0x00000000;
    mm[0].length = 0x00010000;
    mm[0].type   = 0;
    mm[1].size   = 20;
    mm[1].base   = 0x00010000;
    mm[1].length = 0x00090000;
    mm[1].type   = 1;
    mm[2].size   = 20;
    mm[2].base   = 0x000A0000;
    mm[2].length = 0x00060000;
    mm[2].type   = 0;
    mm[3].size   = 20;
    mm[3].base   = 0x00100000;
    mm[3].length = 0x07F00000;
    mm[3].type   = 1;


    uintptr_t entry = load_elf(krnl);


    printf("Load done.\n");
    printf("In AD 2009 fun was beginning.\n");
    printf("Setting up us this program...\n");
    printf("(we have no chance to survive, making our segfault handler)\n");

    init_segfault_handler();

    printf("\n");

    init_sdl();

    asm volatile ("jmp *%%esi" :: "a"(0x2BADB002), "b"(mbi), "S"(entry));

    return EXIT_SUCCESS;
}
