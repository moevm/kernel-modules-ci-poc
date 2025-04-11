#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

int main()
{
	int myppid = getppid();
	printf("%d\n",myppid);

}

