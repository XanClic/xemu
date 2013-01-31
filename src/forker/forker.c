#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/user.h>

#include "forker.h"


#ifndef PTRACE_O_EXITKILL
#define PTRACE_O_EXITKILL 0
#endif


pid_t fork_vm(void)
{
    pid_t child;

    if (!(child = fork()))
    {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        raise(SIGSTOP);

        puts("Entering VM");

        execl("./child", "./child", NULL);
    }


    ptrace(PTRACE_ATTACH, child, NULL, NULL);
    ptrace(PTRACE_SETOPTIONS, child, NULL, (void *)(PTRACE_O_TRACEEXEC | PTRACE_O_EXITKILL));


    return child;
}
