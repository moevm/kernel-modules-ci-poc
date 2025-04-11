#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

int get_myppid(int pid)
{
	char buf[BUFSIZ], comm[BUFSIZ];
	int ppid,myppid,id;
	char state;

	sprintf(buf, "/proc/%d/stat",pid);

	int fd = open(buf,O_RDONLY);

	read(fd,buf,BUFSIZ);
	sscanf(buf, "%d %s %c %d",&id,comm,&state,&myppid);

	close(fd);

	return myppid;
}

int main()
{

	pid_t pid=getpid();

	printf("%d\n",get_myppid(get_myppid(pid)));

}

