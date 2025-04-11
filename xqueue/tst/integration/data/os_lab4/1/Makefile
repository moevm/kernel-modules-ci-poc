#Makefile
.PHONY: all clean

all: solution.o libsolution.so

solution.o: solution.c 
	gcc -fPIC -Wall -g -c solution.c
	gcc -g -shared -Wl,-soname,libsolution.so.0 -o libsolution.so.0.0 solution.o -lc

libsolution.so:	
	ln -sf libsolution.so.0.0 libsolution.so.0
	ln -sf libsolution.so.0 libsolution.so

clean:
	rm -rf *.o *.so*
