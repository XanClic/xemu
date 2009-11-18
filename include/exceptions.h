#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <signal.h>

#define EXCEPTION_GPF 13

void exception(int num, struct sigcontext *);

#endif
