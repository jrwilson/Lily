CXX=g++
OFLAG=-O0
# Add -Werror at some point	
CXXFLAGS=-MD $(OFLAG) -Wall -Wextra -std=c++0x -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -fno-exceptions -fno-stack-protector -I. -I../include
# -nostdinc -fno-rtti
AR=ar

# Loader should be first so the bootloader can find the magic number.
OBJECTS=cpp_runtime.o

LIBCXX=libcxx.a

.PHONY : all
all : $(LIBCXX)

$(LIBCXX) : $(OBJECTS)
	$(AR) -rcs $@ $^

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

