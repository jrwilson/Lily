CC=i586-pc-lily-gcc
CFLAGS=-Wall -O2 -MD -std=c99
PROGRAMS=registry vfs tmpfs init
BIN_PROGRAMS=jsh
#PROGRAMS=keyboard kb_us_104 shell terminal vga
TARGETS=boot_automaton boot_data

.PHONY : all
all : $(TARGETS)

boot_automaton : boot_automaton.o cpio.o vfs_msg.o
	$(CC) -o $@ $^ -lio -ldescription -lbuffer_file -lbuffer_queue -lfifo_scheduler -ldymem -lstring

boot_data : $(PROGRAMS) $(BIN_PROGRAMS) hello.txt
	mkdir -p init_fs
	cp -t init_fs $(PROGRAMS) hello.txt
	mkdir -p init_fs/bin
	cp -t init_fs/bin $(BIN_PROGRAMS)
	(cd init_fs; find . | cpio -o -H newc) > $@

hello.txt :
	echo "Hello world!" > $@

registry : registry.o registry_msg.o
	$(CC) -o $@ $^ -lio -lbuffer_file -lbuffer_queue -lfifo_scheduler -ldymem -lstring

vfs : vfs.o registry_msg.o vfs_msg.o
	$(CC) -o $@ $^ -lio -ldescription -lbuffer_file -lbuffer_queue -lfifo_scheduler -ldymem -lstring

tmpfs : tmpfs.o cpio.o vfs_msg.o
	$(CC) -o $@ $^ -lio -lbuffer_file -lbuffer_queue -lfifo_scheduler -ldymem -lstring

init : init.o registry_msg.o vfs_msg.o
	$(CC) -o $@ $^ -lio -ldescription -lbuffer_file -lbuffer_queue -lcallback_queue -lfifo_scheduler -ldymem -lstring

jsh : jsh.o
	$(CC) -o $@ $^ -lio -lbuffer_queue -lfifo_scheduler -ldymem -lstring

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
	-rm -f $(TARGETS) $(PROGRAMS) *.o
	-rm -rf init_fs

.PHONY : depclean
depclean :
	-rm -f *.d

# Include the dependencies
-include *.d

