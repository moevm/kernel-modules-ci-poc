#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) {
	void* lib_handle;      
	int (*someFunction)(int);
	const char* error_msg;

	char *lib_name = argv[1];
	char *func_name = argv[2];
	int input_multiplier = atoi(argv[3]);

	int tmp = 0;
	char lib_path[1024];
   	if (getcwd(lib_path, sizeof(lib_path)) == NULL)
    	perror("getcwd() error");

    strcat(lib_path, "/");
	strcat(lib_path, lib_name); 
	lib_handle = dlopen(lib_path, RTLD_LAZY);

	if (!lib_handle) {
    	fprintf(stderr, "Error during dlopen(): %s\n", dlerror());
    	exit(1);
	}

	someFunction = (int (*)(int ))dlsym(lib_handle, func_name);

	error_msg = dlerror();
	if (error_msg) {
	    fprintf(stderr, "Error locating %s - %s\n", func_name, error_msg);
	    exit(1);
	}

	tmp = (*someFunction)(input_multiplier);

	printf("%d\n", tmp);
	
	dlclose(lib_handle);
	return 0;
}
