CC=i586-pc-lily-gcc
CFLAGS=-g -Wall -O2 -MD -std=c99
PROGRAMS=syslog vfs tmpfs jsh ps2_keyboard_mouse terminal_server terminal vga pci ps2_keyboard_mouse_test frtimer sampler
# shell
SCRIPTS=start.jsh
TARGETS=boot_automaton boot_data

.PHONY : all
all : $(TARGETS)

boot_automaton : boot_automaton.o cpio.o vfs_msg.o argv.o
	$(CC) -o $@ $^ -ldescription -lbuffer_file -lcallback_queue -ldymem

boot_data : to_buffer_file $(PROGRAMS) $(SCRIPTS)
	mkdir -p init_fs/bin
	cp -t init_fs/bin $(PROGRAMS)
	mkdir -p init_fs/scr
	cp -t init_fs/scr $(SCRIPTS)
	echo -n "xxxx" > $@
	(cd init_fs; find . | cpio -o -H newc) >> $@
	./to_buffer_file $@

to_buffer_file : to_buffer_file.c
	gcc -o $@ $^

# registry : registry.o registry_msg.o
# 	$(CC) -o $@ $^ -lbuffer_file -lbuffer_queue

# shell : shell.o
# 	$(CC) -o $@ $^_buffer -lbuffer_heap

# File systems
vfs : vfs.o vfs_msg.o
	$(CC) -o $@ $^ -lcallback_queue -ldescription -lbuffer_file -ldymem

tmpfs : tmpfs.o cpio.o vfs_msg.o
	$(CC) -o $@ $^ -ldescription -lbuffer_file -ldymem

ps2_keyboard_mouse : ps2_keyboard_mouse.o mouse_msg.o 
	$(CC) -o $@ $^ -lio -ldescription -lbuffer_file

ps2_keyboard_mouse_test : ps2_keyboard_mouse_test.o mouse_msg.o
	$(CC) -o $@ $^ -lbuffer_file

terminal_server : terminal_server.o vga_msg.o mouse_msg.o
	$(CC) -o $@ $^ -lbuffer_file -ldymem

terminal : terminal.o mouse_msg.o
	$(CC) -o $@ $^ -lbuffer_file

vga : vga.o vga_msg.o
	$(CC) -o $@ $^ -lio -ldescription -lbuffer_file

pci : pci.o 
	$(CC) -o $@ $^ -lio -ldescription -lbuffer_file -ldymem

jsh : jsh.o argv.o vfs_msg.o
	$(CC) -o $@ $^ -ldescription -lbuffer_file -lcallback_queue -ldymem

syslog : syslog.o 
	$(CC) -o $@ $^ -lbuffer_file

frtimer : frtimer.o
	$(CC) -o $@ $^ -lbuffer_file

sampler : sampler.o
	$(CC) -o $@ $^ -lbuffer_file

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY : clean
clean :
	-rm -f $(TARGETS) $(PROGRAMS) *.o
	-rm -rf init_fs

.PHONY : depclean
depclean :
	-rm -f *.d

# Include the dependencies
-include *.d

