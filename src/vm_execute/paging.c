#include <stdbool.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "execute.h"
#include "memory.h"
#include "paging.h"
#include "system_state.h"


#define MAP_PR (1 << 0)
#define MAP_RW (1 << 1)
#define MAP_OS (1 << 2)
#define MAP_4M (1 << 7)


// FIXME: Implement OS bit


extern pid_t vm_pid;


void full_paging_update(void)
{
    munmap(adr_g2h(0), 0xfffc000);

    if (vm_pid)
        vm_execute_syscall(91, 2, 0x1000, 0xfffb000);


    if (!(cr[0] & (1 << 31)))
    {
        mmap(adr_g2h(0), MEMSZ, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, 15, 0);

        if (vm_pid)
            vm_execute_syscall(192, 6, 0x1000, MEMSZ - 0x1000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_FIXED, 15, 1);
    }


    // Just leave everything unmapped if paging is enabled. Page faults will bring it in.
}


bool trymap(uintptr_t addr)
{
    if (!(cr[0] & (1 << 31)))
        return false; // no paging, nothing to do


    int pdi =  addr >> 22;
    int pti = (addr >> 12) & 0x3ff;


    uint32_t *page_dir = adr_p2h(cr[3] & ~0x3ff);

    uint32_t pde = page_dir[pdi];

    if (!(pde & MAP_PR))
    {
        if (!pdi)
            vm_execute_syscall(91, 2, 0x1000, (1 << 22) - 0x1000);
        else
            vm_execute_syscall(91, 2, (uint32_t)pdi << 22, 1 << 22);

        munmap(adr_g2h((uintptr_t)pdi << 22), 1 << 22);

        return false;
    }

    if (pde & MAP_4M)
    {
        uint32_t phys = pde & ~0x3fffff;

        if (!pdi)
            vm_execute_syscall(192, 6, 0x1000, (1 << 22) - 0x1000, PROT_READ | PROT_EXEC | ((pde & MAP_RW) ? PROT_WRITE : 0), MAP_SHARED | MAP_FIXED, 15, (phys >> 12) + 1);
        else
            vm_execute_syscall(192, 6, (uint32_t)pdi << 22, 1 << 22, PROT_READ | PROT_EXEC | ((pde & MAP_RW) ? PROT_WRITE : 0), MAP_SHARED | MAP_FIXED, 15, phys >> 12);

        mmap(adr_g2h((uintptr_t)pdi << 22), 1 << 22, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, 15, phys);

        return true;
    }


    uint32_t *page_tbl = adr_p2h(pde & ~0x3ff);

    uint32_t pte = page_tbl[pti];

    if (!(pte & MAP_PR))
    {
        vm_execute_syscall(91, 2, (uint32_t)addr & ~0xfff, 0x1000);

        munmap(adr_g2h((uintptr_t)addr & ~0xfff), 0x1000);

        return false;
    }

    uint32_t phys = pte & ~0xfff;

    vm_execute_syscall(192, 6, (uint32_t)addr & ~0xfff, 0x1000, PROT_READ | PROT_EXEC | ((pte & MAP_RW) ? PROT_WRITE : 0), MAP_SHARED | MAP_FIXED, 15, phys >> 12);

    mmap(adr_g2h((uintptr_t)addr & ~0xfff), 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, 15, phys);


    return true;
}


void invalidate_page(uintptr_t addr)
{
    trymap(addr);
}


int get_page_error_code(siginfo_t *siginfo, struct user_regs_struct *regs)
{
    uintptr_t addr = (uintptr_t)siginfo->si_addr;

    bool present = (siginfo->si_code == SEGV_ACCERR);
    bool infetch = (regs->rip == addr);
    bool user    = (gdt_desc_cache[CS].privilege == 3);

    bool write   = false;


    if (present && !infetch)
    {
        int pdi =  addr >> 22;
        int pti = (addr >> 12) & 0x3ff;

        uint32_t pde = ((uint32_t *)adr_p2h(cr[3] & ~0x3ff))[pdi];

        if (pde & MAP_4M)
            write = !(pde & MAP_RW);
        else
            write = !(((uint32_t *)adr_p2h(pde & ~0x3ff))[pti] & MAP_RW);
    }


    return ((int)infetch << 4) | ((int)user << 2) | ((int)write << 1) | (int)present;
}
