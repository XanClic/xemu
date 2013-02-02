#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/user.h>

#include "forker.h"
#include "memory.h"


#ifndef PTRACE_O_EXITKILL
#define PTRACE_O_EXITKILL 0
#endif


void *vm_comm_area;

pid_t vm_pid;


pid_t fork_vm(uintptr_t entry)
{
    int commfd = shm_open("/xemu_vm_comm", O_RDWR | O_CREAT, 0777);
    dup2(commfd, 14);
    close(commfd);

    ftruncate(14, 4096);

    vm_comm_area = mmap(adr_g2h(0xffffd000), 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, 14, 0);


    *(uint32_t *)vm_comm_area = entry;


    if (!(vm_pid = fork()))
    {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        raise(SIGSTOP);

        puts("Entering VM");

        execl("./child", "./child", NULL);
    }


    ptrace(PTRACE_ATTACH, vm_pid, NULL, NULL);
    ptrace(PTRACE_SETOPTIONS, vm_pid, NULL, (void *)(PTRACE_O_TRACEEXEC | PTRACE_O_EXITKILL));


    return vm_pid;
}
