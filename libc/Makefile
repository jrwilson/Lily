CC=i586-pc-lily-gcc
AS=i586-elf-as
CFLAGS=-MD -O2 -g -Wall -fno-builtin -std=c99

# Headers that will be installed.
HEADERS=automaton.h io.h string.h dymem.h fifo_scheduler.h buffer_queue.h buffer_file.h description.h rr_queue.h

# Loader should be first so the bootloader can find the magic number.
OBJECTS=crt0.o libc.a libstring.a libio.a libdymem.a libfifo_scheduler.a libbuffer_queue.a libbuffer_file.a libdescription.a librr_queue.a

.PHONY : all
all : $(OBJECTS)

libc.a : automaton.o pagesize.o

libstring.a : string.o

libio.a : io.o

libdymem.a : dymem.o

libfifo_scheduler.a : fifo_scheduler.o

libbuffer_queue.a : buffer_queue.o

libbuffer_file.a : buffer_file.o

libdescription.a : description.o

librr_queue.a : rr_queue.o

%.o : %.s
	$(AS) $(ASFLAGS) -o $@ $<

%.o : %.S
	$(CC) $(CFLAGS) -o $@ -c $< 

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $< 

%.a :
	$(AR) cr $@ $^
	ranlib $@

.PHONY : clean
clean :
	-rm -f $(OBJECTS) *.o

.PHONY : depclean
depclean :
	-rm -f *.d

.PHONY : install
install :
	install -d $(PREFIX)/sysroot/usr/include
	install -m 644 -t $(PREFIX)/sysroot/usr/include $(HEADERS)
	install -d $(PREFIX)/sysroot/usr/lib
	install -m 644 -t $(PREFIX)/sysroot/usr/lib $(OBJECTS)

# Include the dependencies
-include *.d
