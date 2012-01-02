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
math.o \
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

.PHONY : iso
iso : lily.iso

initautomaton : lily
	strip lily -o $@

#	echo "This is the initial automaton!!" > $@

initdata :
	echo "This is the initial data!!" > $@

core.img :
	grub-mkimage -p /boot/grub -o $@ biosdisk iso9660 multiboot sh

# core.img :
# 	grub-mkimage -O i386-pc -p /boot/grub -o $@ biosdisk iso9660 multiboot configfile

eltorito.img : core.img
	cat /usr/lib/grub/i386-pc/cdboot.img $^ > $@

lily.iso : eltorito.img grub.cfg lily initautomaton initdata
	mkdir -p isofiles/boot/grub
	cp eltorito.img isofiles/boot/grub/
	cp grub.cfg isofiles/boot/grub/
	cp lily isofiles/boot/
	cp initautomaton isofiles/boot/
	cp initdata isofiles/boot/
	genisoimage -R -b boot/grub/eltorito.img -no-emul-boot -boot-load-size 4 -boot-info-table -o $@ isofiles

.PHONY : isoclean
isoclean :
	-rm -f initautomaton initdata core.img eltorito.img lily.iso

# Include the dependencies
-include *.d

