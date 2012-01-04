AS=as
CXX=g++
OFLAG=-O0
# Add -Werror at some point	
CXXFLAGS=-MD $(OFLAG) -g -Wall -Wextra -std=c++0x -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -fno-exceptions -fno-stack-protector -I. -I../include
# -nostdinc -fno-rtti
LD=ld
LDFLAGS=$(OFLAG) -T linker.ld

TARGETS=init_automaton

.PHONY : all
all : $(TARGETS)

init_automaton : init_automaton.o
	$(LD) $(LDFLAGS) -o $@ $^

%.o : %.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS)

.PHONY : clean
clean :
	-rm -f $(TARGETS) *.o

.PHONY : depclean
depclean :
	-rm -f *.d

# Include the dependencies
-include *.d

