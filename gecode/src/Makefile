CPP		= g++
LDPATH  = /usr/local/lib
INCPATH = /usr/local/include
OS = $(shell uname)

ifeq ($(OS),Darwin)
OPTIONS = -F/Library/Frameworks -stdlib=libstdc++

all: warehouse

warehouse.o: warehouse.cpp
	$(CPP) $(OPTIONS) -c $< -o $@

warehouse: warehouse.o
	$(CPP) $(OPTIONS) $< -o $@ -framework gecode


clean:
	rm -f *.o

else
OPTIONS = -lgecodesearch -lgecodeint -lgecodedriver -lgecodeminimodel -lgecodegist -lgecodekernel -lgecodesupport

all: warehouse

warehouse.o: warehouse.cpp
	$(CPP) -I$(INCPATH) -c $< -o $@

warehouse: warehouse.o
	$(CPP) -L$(LDPATH) $< -o $@ $(OPTIONS)


clean:
	rm -f *.o

endif
