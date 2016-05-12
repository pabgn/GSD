CPP		= g++
LDPATH  = /usr/local/lib
INCPATH = /usr/local/include
OBJDIR = obj
OS = $(shell uname)

vpath %.cpp src
vpath %.o obj


ifeq ($(OS),Darwin)
OPTIONS = -F/Library/Frameworks -stdlib=libstdc++

all: warehouse

$(OBJDIR)/warehouse.o: warehouse.cpp
	$(CPP) $(OPTIONS) -c $< -o $@

warehouse: warehouse.o
	$(CPP) $(OPTIONS) $< -o $@ -framework gecode


docs: doc/src/Makefile
	$(MAKE) -C doc/src/ all
	cp doc/src/*.pdf doc/pdf/

clean:
	rm -f obj/*.o

else
OPTIONS = -lgecodesearch -lgecodeint -lgecodedriver -lgecodeminimodel -lgecodegist -lgecodekernel -lgecodesupport

all: warehouse

$(OBJDIR)/warehouse.o: warehouse.cpp
	$(CPP) -I$(INCPATH) -c $< -o $@

warehouse: warehouse.o
	$(CPP) -L$(LDPATH) $< -o $@ $(OPTIONS)

docs: doc/src/Makefile
	$(MAKE) -C doc/src/ all
	cp doc/src/*.pdf doc/pdf/

clean:
	rm -f obj/*.o

endif
