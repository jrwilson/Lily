.PHONY : all
all : lily.iso

kernel/klily :
	cd kernel; $(MAKE) $(MFLAGS)

user/boot_automaton :
	cd user; $(MAKE) $(MFLAGS)

user/boot_data :
	cd user; $(MAKE) $(MFLAGS)

.PHONY : clean
clean :
	cd kernel; $(MAKE) $(MFLAGS) clean
	cd user; $(MAKE) $(MFLAGS) clean

.PHONY : depclean
depclean :
	cd kernel; $(MAKE) $(MFLAGS) depclean
	cd user; $(MAKE) $(MFLAGS) depclean

klily : kernel/klily
	strip $^ -o $@

boot_automaton : user/boot_automaton
	strip $^ -o $@

boot_data : user/boot_data
	cp $^ $@

# core.img :
# 	grub-mkimage -p /boot/grub -o $@ biosdisk iso9660 multiboot sh

core.img :
	grub-mkimage -O i386-pc -p /boot/grub -o $@ biosdisk iso9660 multiboot configfile

eltorito.img : core.img
	cat /usr/lib/grub/i386-pc/cdboot.img $^ > $@

lily.iso : eltorito.img grub.cfg klily boot_automaton boot_data
	mkdir -p isofiles/boot/grub
	cp eltorito.img isofiles/boot/grub/
	cp grub.cfg isofiles/boot/grub/
	cp klily isofiles/boot/
	cp boot_automaton isofiles/boot/
	cp boot_data isofiles/boot/
	genisoimage -R -b boot/grub/eltorito.img -no-emul-boot -boot-load-size 4 -boot-info-table -o $@ isofiles

.PHONY : isoclean
isoclean :
	-rm -f boot_automaton boot_data core.img eltorito.img lily.iso

# Include the dependencies
-include *.d

