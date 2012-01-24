CC=i586-pc-lily-gcc
AS=i586-elf-as
CFLAGS=-MD -O2 -g -Wall -std=c99

# Headers that will be installed.
HEADERS=action.h buffer.h finish.h io.h string.h dymem.h

# Loader should be first so the bootloader can find the magic number.
OBJECTS=crt0.o libc.a libstring.a libbuffer.a libio.a libdymem.a libsysconf.a

.PHONY : all
all : $(OBJECTS)

libc.a : finish.o

libstring.a : string.o

libbuffer.a : buffer.o

libio.a : io.o

libdymem.a : dymem.o sbrk.o

libsysconf.a : sysconf.o

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