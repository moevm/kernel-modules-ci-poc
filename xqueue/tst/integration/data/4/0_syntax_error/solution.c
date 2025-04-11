#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

Hello, I am a syntax error

int main()
{
	char buf[BUFSIZ], comm[BUFSIZ];
	int ppid,myppid,id;
	char state;

	pid_t pid=getpid();

	sprintf(buf, "/proc/%d/stat",pid);

	int fd = open(buf,O_RDONLY);

	read(fd,buf,BUFSIZ);
	sscanf(buf, "%d %s %c %d",&id,comm,&state,&myppid);

	printf("%d\n",myppid);

	close(fd);
}

