#include <stdio.h>
#include <string.h>

int stringStat(const char *string, int *multiplier, int *count){
    *count+=1;
    return (*multiplier)*(strlen(string));
}