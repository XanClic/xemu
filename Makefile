CC = @gcc
CFLAGS = -Iinclude -std=gnu99 -Wall -pedantic -g2 -m32
LINK = @gcc
LFLAGS = -Xlinker -Ttext -Xlinker 0x70000000 -m32 `sdl-config --cflags --libs`
ASM = @fasm > /dev/null
PBC = /usr/bin/pbcompiler
DEL = -@rm &> /dev/null

.PHONY: all clean pb

all: $(patsubst %.c,%.o,$(wildcard *.c)) $(patsubst %.c,%.o,$(wildcard devices/*.c)) $(patsubst %.asm,%.o,$(wildcard *.asm))
	@echo "LING   -> xemu"
	$(LINK) $(LFLAGS) *.o devices/*.o -o xemu

%.o: %.c
	@echo "CC     $<"
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.asm
	@echo "ASM    $<"
	$(ASM) $< $@

pb: $(wildcard *.pb)
	@echo "Creating CMD script creator..."
	@echo "#!/bin/bash" > make_cmd_script
	@echo -n "echo '#!/bin/bash' > " >> make_cmd_script
	@echo -n $$ >> make_cmd_script
	@echo 1 >> make_cmd_script
	@echo -n "echo 'export PATH=" >> make_cmd_script
	@echo -n $$ >> make_cmd_script
	@echo -n "OLD_PATH' >> " >> make_cmd_script
	@echo -n $$ >> make_cmd_script
	@echo 1 >> make_cmd_script
	@echo -n "echo " >> make_cmd_script
	@echo -n $$ >> make_cmd_script
	@echo -n "@ '" >> make_cmd_script
	@echo -n $$ >> make_cmd_script
	@echo -n "@' >> " >> make_cmd_script
	@echo -n $$ >> make_cmd_script
	@echo 1 >> make_cmd_script
	@echo -n "chmod a+x " >> make_cmd_script
	@echo -n $$ >> make_cmd_script
	@echo 1 >> make_cmd_script
	@chmod a+x make_cmd_script

	@echo "Creating scripts for rm, gcc, strip and so on..."
	@./make_cmd_script rm
	@./make_cmd_script gcc -Xlinker -Ttext -Xlinker 0x70000000
	@./make_cmd_script strip
	@./make_cmd_script sdl-config
	@./make_cmd_script pkg-config

	@echo "Running pbcopiler..."
	@export OLD_PATH="$$PATH" && export PATH=$(shell pwd) && $(PBC) -e xemupb xemu.pb

	@echo "Cleaning up..."
	$(DEL) rm gcc strip sdl-config pkg-config make_cmd_script

clean:
	$(DEL) *.o devices/*.o xemu xemupb
