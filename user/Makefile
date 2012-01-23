CC=i586-pc-lily-gcc
CFLAGS=-g -Wall -MD -std=c99
TARGETS=init_automaton init_data

.PHONY : all
all : $(TARGETS)

init_automaton : init_automaton.o
	$(CC) -o $@ $^

init_data :
	mkdir -p init_fs
	(cd init_fs; find . | cpio -o -H newc) > $@

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY : clean
clean :
	-rm -f $(TARGETS) *.o

.PHONY : depclean
depclean :
	-rm -f *.d

# Include the dependencies
-include *.d

