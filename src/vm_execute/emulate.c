#include <stdbool.h>
#include <sys/user.h>

#include "execute.h"


bool emulate(struct user_regs_struct *regs)
{
    (void)regs;

    return false;
}
