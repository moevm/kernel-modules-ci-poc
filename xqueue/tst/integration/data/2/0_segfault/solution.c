#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>


int main (int argc, char* argv[])
{
    int a = atoi(argv[3]);
    int (*someSecretFunctionPrototype)(int);
    void *hdl = dlopen(argv[1], RTLD_LAZY);
    someSecretFunctionPrototype = (int (*)(int))dlsym(hdl,"someSecretFunctionPrototype");
    int b = someSecretFunctionPrototype(a);
    printf("%d\n",b);
    return 0;
}