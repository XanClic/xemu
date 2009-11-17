#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <signal.h>
#include <stdint.h>

void init_instructions(void);

int far_jmp(uint8_t *instr_base, struct sigcontext *ctx);
int lgdt_lidt(uint8_t *instr_base, struct sigcontext *ctx);
int ltr_lldt(uint8_t *instr_base, struct sigcontext *ctx);
int mov_sreg_reg(uint8_t *instr_base, struct sigcontext *ctx);
int out_dx_al(uint8_t *instr_base, struct sigcontext *ctx);
int sti(uint8_t *instr_base, struct sigcontext *ctx);
int two_byte_instr(uint8_t *instr_base, struct sigcontext *ctx);

#endif
