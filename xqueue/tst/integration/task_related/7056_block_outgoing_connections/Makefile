#Makefile
.PHONY: all clean

all: solution.o 

solution.o: solution.c 
	ping -c 3 -W 2 8.8.8.8

clean:
	rm -rf *.o *.so*
