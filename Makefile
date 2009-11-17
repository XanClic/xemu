CC = @gcc
CFLAGS = -Iinclude -std=gnu99 -Wall -pedantic -g2
LINK = @gcc
LFLAGS = -Xlinker -Ttext -Xlinker 0x70000000 /usr/lib/libSDL_image-1.2.so.0 `sdl-config --cflags --libs`
ASM = @fasm > /dev/null
PBC = /usr/bin/pbcompiler
DEL = -@rm &> /dev/null

.PHONY: all clean pb

all: $(patsubst %.c,%.o,$(wildcard *.c)) $(patsubst %.asm,%.o,$(wildcard *.asm))
	@echo "LING   -> xemu"
	$(LINK) $(LFLAGS) *.o -o xemu

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

	@echo "Creating scripts for rm, gcc and strip..."
	@./make_cmd_script rm
	@./make_cmd_script gcc -Xlinker -Ttext -Xlinker 0x70000000
	@./make_cmd_script strip

	@echo "Running pbcopiler..."
	@export OLD_PATH="$$PATH" && export PATH=$(shell pwd) && $(PBC) -e xemupb xemu.pb

	@echo "Cleaning up..."
	$(DEL) rm
	$(DEL) gcc
	$(DEL) strip
	$(DEL) make_cmd_script

clean:
	$(DEL) *.o xemu xemupb
