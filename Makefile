AS=nasm
AFLAGS=-f elf
CC=gcc
# Add -Werror at some point
CFLAGS=-Wall -Wextra -nostdlib -fno-builtin -nostartfiles -nodefaultlibs
LD=ld

# Loader should be first so the bootloader can find the magic number.
OBJECTS=loader.o kernel.o kput.o memory.o interrupt.o isr.o io.o

KERNEL=lily

.PHONY : all
all : $(KERNEL)

$(KERNEL) : $(OBJECTS)
	$(LD) -T linker.ld -o $@ $^

%.o : %.s
	$(AS) $(AFLAGS) -o $@ $<

%.o : %.c
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY : clean
clean :
	-rm -f $(KERNEL) $(OBJECTS)

.PHONY : iso
iso : lily.iso

#core.img :
#	grub-mkimage -p /boot/grub -o $@ biosdisk iso9660 multiboot sh

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