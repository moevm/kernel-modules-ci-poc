#include <stdio.h>
#include <string.h>

Hello, I am a WRONGCODE

int stringStat(const char *string, size_t multiplier, int *count) {
	*count += 1;
	return strlen(string) * multiplier;
}
