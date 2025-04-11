#Makefile
.PHONY: all clean

TARGET = solution

all: $(TARGET)

solution.o: solution.c
	gcc -Wall -g -c solution.c 

$(TARGET): solution.o 
	gcc -g -o solutio1n solution.o -ldl

clean:
	rm -rf $(TARGET) *.o *.so*
