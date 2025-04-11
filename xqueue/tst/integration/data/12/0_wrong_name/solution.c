#include <stdio.h>
#include <strings.h>
#include <sys/select.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>


int usr1=0;
int usr2=0;


void h_usr1(int s)
{
	usr1++;
}


void h_usr2(int s)
{
	usr2++;
}


void h_term(int s)
{
	printf("%d %d\n",usr1,usr2);
//	exit(0);
}



int main(int c, char **v)
{

	signal(SIGUSR1,h_usr1);
	signal(SIGUSR2,h_usr2);
	signal(SIGTERM,h_term);

	while(1) usleep(1000);
	return 0;
}
