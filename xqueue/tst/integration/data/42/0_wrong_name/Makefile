#Makefile
.PHONY: all clean

TARGET = solution1

all: $(TARGET)

solution.o: solution.c
	gcc -Wall -g -c solution.c 

$(TARGET): solution.o 
	gcc -g -o solution1 solution.o -ldl

clean:
	rm -rf $(TARGET) *.o *.so*
