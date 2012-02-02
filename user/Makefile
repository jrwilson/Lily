CC=i586-pc-lily-gcc
CFLAGS=-g -Wall -O2 -MD -std=c99
PROGRAMS=producer consumer printer console vga
TARGETS=init_automaton init_data

.PHONY : all
all : $(TARGETS)

init_automaton : init_automaton.o
	$(CC) -o $@ $^ -lstring -ldymem

init_data : $(PROGRAMS)
	mkdir -p init_fs
	cp -t init_fs $^
	(cd init_fs; find . | cpio -o -H newc) > $@


producer : producer.o
	$(CC) -o $@ $^ -lstring

consumer : consumer.o
	$(CC) -o $@ $^ -lio -ldymem -lstring

printer : printer.o
	$(CC) -o $@ $^ -lfifo_scheduler -ldymem -lstring

console : console.o
	$(CC) -o $@ $^ -lstring -lfifo_scheduler -ldymem

vga : vga.o
	$(CC) -o $@ $^ -lio -ldymem -lstring

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

