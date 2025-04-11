#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

hello i am syntax error

int get_ppid(int pid) 
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

int main(int c, char **v)
{
	if(c!=2)
		return -1;

	int pid = atoi(v[1]);

	while	(pid >=1)
	{
		printf("%d\n",pid);
		pid = get_ppid(pid);
	}
}

