#ifndef PAGING_H
#define PAGING_H

#include <stdbool.h>


void full_paging_update(void);

bool trymap(uintptr_t addr);

#endif
