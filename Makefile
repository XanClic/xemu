CC = gcc
CFLAGS = -I. -std=gnu99 -Wall -pedantic -g2
LINK = gcc
LFLAGS = -Xlinker -Ttext -Xlinker 0x70000000
ASM = fasm

.PHONY: all clean

all: $(patsubst %.c,%.o,$(wildcard *.c)) $(patsubst %.asm,%.o,$(wildcard *.asm))
	$(LINK) $(LFLAGS) *.o -o xemu

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.asm
	$(ASM) $< $@

clean:
	@rm *.o
