#ifndef FORKER_H
#define FORKER_H

#include <stdint.h>
#include <sys/types.h>


pid_t fork_vm(uintptr_t entry);

#endif
