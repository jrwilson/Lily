AS=nasm
AFLAGS=-f elf
CXX=time g++
# Add -Werror at some point
CXXFLAGS=-MD -Wall -Wextra -nostdlib -fno-builtin -nostartfiles -nostdinc -nodefaultlibs -fno-exceptions -fno-rtti -fno-stack-protector -I. -I stl
LD=ld

# Loader should be first so the bootloader can find the magic number.
OBJECTS=loader.o \
gdt_flush.o \
idt_flush.o \
exception.o \
exception_handler.o \
irq.o \
irq_handler.o \
trap.o \
trap_handler.o \
placement_allocator.o \
stack_allocator.o \
frame.o \
frame_manager.o \
vm_manager.o \
kput.o \
halt.o \
cpp_runtime.o \
string.o \
list_allocator.o \
syscall.o \
global_descriptor_table.o \
system_automaton.o \
initrd_automaton.o \
automaton.o \
scheduler.o \
interrupt_descriptor_table.o \
kmain.o

# pit.o \
# count_to_ten.o \
# producer.o \
# consumer.o

KERNEL=lily

.PHONY : all
all : $(KERNEL)

$(KERNEL) : $(OBJECTS)
	$(LD) -T linker.ld -o $@ $^

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

.PHONY : iso
iso : lily.iso

# core.img :
# 	grub-mkimage -p /boot/grub -o $@ biosdisk iso9660 multiboot sh

core.img :
	grub-mkimage -O i386-pc -p /boot/grub -o $@ biosdisk iso9660 multiboot configfile

eltorito.img : core.img
	cat /usr/lib/grub/i386-pc/cdboot.img $^ > $@

lily.iso : eltorito.img lily
	mkdir -p isofiles/boot/grub
	cp eltorito.img isofiles/boot/grub/
	cp lily isofiles/boot/
	genisoimage -R -b boot/grub/eltorito.img -no-emul-boot -boot-load-size 4 -boot-info-table -o $@ isofiles

.PHONY : isoclean
isoclean :
	-rm -f core.img eltorito.img lily.iso

# Include the dependencies
-include *.d

