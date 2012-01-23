.PHONY : all
all : lily.iso

kernel/klily :
	cd kernel; $(MAKE) $(MFLAGS)

user/init_automaton :
	cd user; $(MAKE) $(MFLAGS)

user/init_data :
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

initautomaton : user/init_automaton
	strip $^ -o $@

initdata : user/init_data
	cp $^ $@

core.img :
	grub-mkimage -p /boot/grub -o $@ biosdisk iso9660 multiboot sh

# core.img :
# 	grub-mkimage -O i386-pc -p /boot/grub -o $@ biosdisk iso9660 multiboot configfile

eltorito.img : core.img
	cat /usr/lib/grub/i386-pc/cdboot.img $^ > $@

lily.iso : eltorito.img grub.cfg klily initautomaton initdata
	mkdir -p isofiles/boot/grub
	cp eltorito.img isofiles/boot/grub/
	cp grub.cfg isofiles/boot/grub/
	cp klily isofiles/boot/
	cp initautomaton isofiles/boot/
	cp initdata isofiles/boot/
	genisoimage -R -b boot/grub/eltorito.img -no-emul-boot -boot-load-size 4 -boot-info-table -o $@ isofiles

.PHONY : isoclean
isoclean :
	-rm -f initautomaton initdata core.img eltorito.img lily.iso

# Include the dependencies
-include *.d

