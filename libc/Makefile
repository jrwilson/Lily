CC=i586-pc-lily-gcc
# AS=i586-elf-as
# CXX=i586-elf-g++
# OFLAG=-O2
# # Add -Werror at some point	
# CXXFLAGS=-MD $(OFLAG) -g -Wall -Wextra -ffreestanding -fno-rtti -fno-exceptions -fno-stack-protector

# # -std=c++0x -ffreestanding -fno-exceptions -fno-stack-protector -fno-rtti -I. -I../include -I/usr/include -I/usr/include/c++/4.4 -I/usr/include/c++/4.4/i486-linux-gnu
# # -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -fno-exceptions -fno-stack-protector -I. -I../include -I/usr/include -I/usr/include/c++/4.4 -I/usr/include/c++/4.4/i486-linux-gnu
# # -nostdinc -fno-rtti
# LD=ld
# LDFLAGS=$(OFLAG) -T linker.ld

# Headers that will be installed.
HEADERS=action.h finish.h

# Loader should be first so the bootloader can find the magic number.
OBJECTS=crt0.o libc.a

.PHONY : all
all : $(OBJECTS)

%.o : %.s
	$(AS) $(ASFLAGS) -o $@ $<

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $< 

libc.a : finish.o
	$(AR) cr libc.a $^
	ranlib libc.a

.PHONY : clean
clean :
	-rm -f $(OBJECTS)

.PHONY : depclean
depclean :
	-rm -f *.d

.PHONY : install
install :
	install -d $(PREFIX)/sysroot/usr/include
	install -m 644 -t $(PREFIX)/sysroot/usr/include $(HEADERS)
	install -d $(PREFIX)/sysroot/usr/lib
	install -m 644 -t $(PREFIX)/sysroot/usr/lib $(OBJECTS)

# # Include the dependencies
# -include *.d
