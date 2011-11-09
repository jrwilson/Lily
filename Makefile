AS=nasm
AFLAGS=-f elf
CC=gcc
# Add -Werror at some point
CFLAGS=-Wall -Wextra -nostdlib -fno-builtin -nostartfiles -nodefaultlibs
LD=ld

# Loader should be first so the bootloader can find the magic number.
OBJECTS=loader.o \
kernel.o \
io.o \
kput.o \
halt.o \
descriptor.o \
gdt_asm.o \
gdt.o \
idt_asm.o \
idt.o \
multiboot_preparse.o \
placement_allocator.o \
frame_manager.o \
multiboot_parse.o \
vm_manager.o \
syscall_handler.o \
scheduler.o \
vm_area.o \
automaton.o \
binding_manager.o \
system_automaton.o \
syscall.o \
list_allocator.o \
string.o \
fifo_scheduler.o \
hash_map.o \
page_fault_handler.o \
initrd_automaton.o

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

%.o : %.c
	$(CC) -g -o $@ -c $< $(CFLAGS)

.PHONY : clean
clean :
	-rm -f $(KERNEL) $(OBJECTS)

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