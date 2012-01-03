.PHONY : all
all : libnote.a

libnote.a : note.o
	ar -rcs libnote.a $^

%.o : %.cpp
	g++ -MD -g -o $@ -c $<

.PHONY : clean
clean :
	-rm -f libnote.a note.o

.PHONY : depclean
depclean :
	-rm -f *.d

# Include the dependencies
-include *.d

