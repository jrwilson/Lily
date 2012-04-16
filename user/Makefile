CC=i586-pc-lily-gcc
CFLAGS=-g -Wall -O2 -MD -std=c99
PROGRAMS=\
syslog \
vfs \
tmpfs \
jsh \
ps2_keyboard_mouse \
terminal \
vga \
pci \
ps2_keyboard_mouse_test \
frtimer \
sampler \
ne2000 \
serial_port \
byte_channel \
ecma2unix \
zsnoop \
bios

SCRIPTS=start.jsh
TARGETS=boot_automaton boot_data

.PHONY : all
all : $(TARGETS)

boot_automaton : boot_automaton.o cpio.o vfs_msg.o argv.o
	$(CC) -o $@ $^ -ldescription -lbuffer_file -lcallback_queue -ldymem -lstring

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
	$(CC) -o $@ $^ -lcallback_queue -ldescription -lbuffer_file -ldymem -lstring

tmpfs : tmpfs.o cpio.o vfs_msg.o
	$(CC) -o $@ $^ -ldescription -lbuffer_file -ldymem -lstring

ps2_keyboard_mouse : ps2_keyboard_mouse.o mouse_msg.o 
	$(CC) -o $@ $^  -ldescription -lbuffer_file -lstring

ps2_keyboard_mouse_test : ps2_keyboard_mouse_test.o mouse_msg.o
	$(CC) -o $@ $^ -lbuffer_file -lstring

terminal : terminal.o vga_msg.o mouse_msg.o
	$(CC) -o $@ $^ -ldescription -lbuffer_file -ldymem -lstring

vga : vga.o vga_msg.o
	$(CC) -o $@ $^  -ldescription -lbuffer_file -lstring

pci : pci.o 
	$(CC) -o $@ $^  -ldescription -lbuffer_file -ldymem -lstring

jsh : jsh.o argv.o vfs_msg.o
	$(CC) -o $@ $^ -ldescription -lbuffer_file -lcallback_queue -ldymem -lstring

syslog : syslog.o 
	$(CC) -o $@ $^ -lbuffer_file -lstring

frtimer : frtimer.o
	$(CC) -o $@ $^ -lbuffer_file -lstring

sampler : sampler.o
	$(CC) -o $@ $^ -lbuffer_file -lstring

ne2000 : ne2000.o
	$(CC) -o $@ $^ -ldescription -lbuffer_file -lstring

serial_port : serial_port.o
	$(CC) -o $@ $^ -ldescription -lbuffer_file  -ldymem -lstring

byte_channel : byte_channel.o
	$(CC) -o $@ $^ -lbuffer_file -lstring

ecma2unix : ecma2unix.o mouse_msg.o
	$(CC) -o $@ $^ -lbuffer_file -lstring

zsnoop : zsnoop.o
	$(CC) -o $@ $^ -lbuffer_file -lstring

bios : bios.o vfs_msg.o
	$(CC) -o $@ $^ -lcallback_queue -ldescription -lbuffer_file  -ldymem -lstring

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

