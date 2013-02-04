#ifndef PAGING_H
#define PAGING_H

#include <signal.h>
#include <stdbool.h>
#include <sys/user.h>


void full_paging_update(void);

bool trymap(uintptr_t addr);

int get_page_error_code(siginfo_t *siginfo, struct user_regs_struct *regs);

#endif
