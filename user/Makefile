CC=i586-pc-lily-gcc
CFLAGS=-g -Wall -O2 -MD -std=c99
BOOT_PROGRAMS=registry vfs tmpfs init
PROGRAMS=keyboard kb_us_104 shell terminal vga
TARGETS=boot_automaton boot_data

.PHONY : all
all : $(TARGETS)

boot_automaton : boot_automaton.o cpio.o
	$(CC) -o $@ $^ -lio -lbuffer_file -lfifo_scheduler -ldymem -lstring

# $(PROGRAMS)
boot_data : $(BOOT_PROGRAMS)
	mkdir -p init_fs
	cp -t init_fs $^
	(cd init_fs; find . | cpio -o -H newc) > $@

registry : registry.o
	$(CC) -o $@ $^ -lio -lbuffer_file -lbuffer_queue -lfifo_scheduler -ldymem -lstring

vfs : vfs.o
	$(CC) -o $@ $^ -lio -lbuffer_file -lbuffer_queue -lfifo_scheduler -ldymem -lstring

tmpfs : tmpfs.o cpio.o
	$(CC) -o $@ $^ -lio -lbuffer_file -lbuffer_queue -lfifo_scheduler -ldymem -lstring

init : init.o
	$(CC) -o $@ $^


keyboard : keyboard.o
	$(CC) -o $@ $^ -lio -lstring_buffer -lbuffer_heap -lfifo_scheduler -ldymem -lstring

kb_us_104 : kb_us_104.o
	$(CC) -o $@ $^ -lstring_buffer -lbuffer_heap -lfifo_scheduler -ldymem -lstring

shell : shell.o
	$(CC) -o $@ $^ -lstring_buffer -lbuffer_heap -lfifo_scheduler -ldymem -lstring

terminal : terminal.o
	$(CC) -o $@ $^ -lstring -lfifo_scheduler -ldymem -lstring_buffer -lbuffer_heap

vga : vga.o
	$(CC) -o $@ $^ -lio -ldymem -lstring -lbuffer_heap

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY : clean
clean :
	-rm -f $(TARGETS) $(BOOT_PROGRAMS) $(PROGRAMS) *.o
	-rm -rf init_fs

.PHONY : depclean
depclean :
	-rm -f *.d

# Include the dependencies
-include *.d

