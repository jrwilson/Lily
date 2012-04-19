CC=i586-pc-lily-gcc
AS=i586-pc-lily-as
CFLAGS=-MD -O2 -g -Wall -fno-builtin -std=c99

# Headers that will be installed.
HEADERS=automaton.h string.h ctype.h dymem.h buffer_file.h description.h callback_queue.h

# Loader should be first so the bootloader can find the magic number.
OBJECTS=crt0.o libc.a libdymem.a libbuffer_file.a libdescription.a libcallback_queue.a

.PHONY : all
all : $(OBJECTS)

libc.a : automaton.o string.o ctype.o printf.o

libdymem.a : dymem.o

libbuffer_file.a : buffer_file.o

libdescription.a : description.o

libcallback_queue.a : callback_queue.o

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

.PHONY : install-headers
install-headers :
	install -d $(PREFIX)/sysroot/usr/include
	install -m 644 -t $(PREFIX)/sysroot/usr/include $(HEADERS)

.PHONY : install
install :
	install -d $(PREFIX)/sysroot/usr/lib
	install -m 644 -t $(PREFIX)/sysroot/usr/lib $(OBJECTS)

# Include the dependencies
-include *.d
