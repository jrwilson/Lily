AS=nasm
AFLAGS=-f elf
CXX=g++
OFLAG=-O0
# Add -Werror at some point	
CXXFLAGS=-MD $(OFLAG) -Wall -Wextra -std=c++0x -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -fno-exceptions -fno-stack-protector -I. -I ltl -I../include
# -nostdinc -fno-rtti
LD=ld
LDFLAGS=$(OFLAG) -T linker.ld

# Loader should be first so the bootloader can find the magic number.
OBJECTS=loader.o \
cpp_runtime.o \
string.o \
ceil.o \
kout.o \
kernel_allocator.o \
gdt.o \
gdt_flush.o \
idt.o \
exception_handler.o \
exception.o \
irq_handler.o \
irq.o \
trap_handler.o \
trap.o \
stack_allocator.o \
frame_manager.o \
vm.o \
global_fifo_scheduler.o \
kmain.o \
system_automaton.o

#note.o
#action_test_automaton.o \
#buffer_test_automaton.o \
#ramdisk_automaton.o \
#ext2_automaton.o \
#boot_automaton.o
# pit.o \

KERNEL=lily

.PHONY : all
all : $(KERNEL)

$(KERNEL) : $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o : %.s
	$(AS) $(AFLAGS) -o $@ $<

%.o : %.cpp
	$(CXX) -g -o $@ -c $< $(CXXFLAGS)

.PHONY : clean
clean :
	-rm -f $(KERNEL) $(OBJECTS)

.PHONY : depclean
depclean :
	-rm -f *.d

# Include the dependencies
-include *.d

