CC=i586-pc-lily-gcc
CFLAGS=-Wall -O2 -MD -std=c99
PROGRAMS=vfs tmpfs jsh keyboard
# kb_us_104 shell terminal vga
SCRIPTS=start.jsh
TARGETS=boot_automaton boot_data

.PHONY : all
all : $(TARGETS)

boot_automaton : boot_automaton.o cpio.o vfs_msg.o argv.o
	$(CC) -o $@ $^ -lio -ldescription -lbuffer_file -lbuffer_queue -lcallback_queue -lfifo_scheduler -ldymem -lstring

boot_data : $(PROGRAMS) $(SCRIPTS)
	mkdir -p init_fs/bin
	cp -t init_fs/bin $(PROGRAMS)
	mkdir -p init_fs/scr
	cp -t init_fs/scr $(SCRIPTS)
	(cd init_fs; find . | cpio -o -H newc) > $@

# registry : registry.o registry_msg.o
# 	$(CC) -o $@ $^ -lio -lbuffer_file -lbuffer_queue -lfifo_scheduler -ldymem -lstring

vfs : vfs.o vfs_msg.o
	$(CC) -o $@ $^ -lio -ldescription -lbuffer_file -lbuffer_queue -lfifo_scheduler -ldymem -lstring

tmpfs : tmpfs.o cpio.o vfs_msg.o
	$(CC) -o $@ $^ -lio -lbuffer_file -lbuffer_queue -lfifo_scheduler -ldymem -lstring

jsh : jsh.o argv.o vfs_msg.o
	$(CC) -o $@ $^ -lio -ldescription -lbuffer_file -lbuffer_queue -lcallback_queue -lfifo_scheduler -ldymem -lstring

keyboard : keyboard.o
	$(CC) -o $@ $^ -lio -lbuffer_queue -lbuffer_file -lfifo_scheduler -ldymem -lstring

# kb_us_104 : kb_us_104.o
# 	$(CC) -o $@ $^ -lstring_buffer -lbuffer_heap -lfifo_scheduler -ldymem -lstring

# shell : shell.o
# 	$(CC) -o $@ $^ -lstring_buffer -lbuffer_heap -lfifo_scheduler -ldymem -lstring

# terminal : terminal.o
# 	$(CC) -o $@ $^ -lstring -lfifo_scheduler -ldymem -lstring_buffer -lbuffer_heap

# vga : vga.o
# 	$(CC) -o $@ $^ -lio -ldymem -lstring -lbuffer_heap

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

