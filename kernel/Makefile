AS=i586-elf-as
CXX=i586-elf-g++
OFLAG=-O2
# Add -Werror at some point	
CXXFLAGS=-MD $(OFLAG) -g -Wall -Wextra -ffreestanding -fno-rtti -fno-exceptions -fno-stack-protector

# -std=c++0x -ffreestanding -fno-exceptions -fno-stack-protector -fno-rtti -I. -I../include -I/usr/include -I/usr/include/c++/4.4 -I/usr/include/c++/4.4/i486-linux-gnu
# -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -fno-exceptions -fno-stack-protector -I. -I../include -I/usr/include -I/usr/include/c++/4.4 -I/usr/include/c++/4.4/i486-linux-gnu
# -nostdinc -fno-rtti
LD=ld
LDFLAGS=$(OFLAG) -T linker.ld

# Loader should be first so the bootloader can find the magic number.
OBJECTS=loader.o \
kmain.o \
kout.o \
kernel_allocator.o \
frame_manager.o \
stack_allocator.o \
cpp_runtime.o \
gdt.o \
gdt_flush.o \
idt.o \
exception_handler.o \
exception.o \
irq_handler.o \
irq.o \
trap_handler.o \
trap.o \
rts.o \
global_fifo_scheduler.o

KERNEL=lily

.PHONY : all
all : $(KERNEL)

$(KERNEL) : $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o : %.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS)

%.o : %.S
	$(AS) -o $@ $<

.PHONY : clean
clean :
	-rm -f $(KERNEL) $(OBJECTS)

.PHONY : depclean
depclean :
	-rm -f *.d

# Include the dependencies
-include *.d

