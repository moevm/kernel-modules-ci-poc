#include <stdio.h>
#include <strings.h>
#include <sys/select.h>
#include <fcntl.h>
#include <string.h>


int main(int c, char **v)
{
	int fd1 = open("./in1",O_RDONLY);
	int fd2 = open("./in2",O_RDONLY);

	char buf[BUFSIZ];

	if( !fd1 || !fd2 )
	{
		perror("Failed to make pipes. Have created in1 in2 before call?\n");
		return -1;
	}
	
	fd_set rd,wd,xd;

	int limit = 20;
	int sum=0;

	while(fd1 || fd2)
	{
	  FD_ZERO(&rd);
  	FD_ZERO(&xd);
		FD_ZERO(&wd);

		if(0!=fd1) 
			FD_SET(fd1,&rd);
		
		if(0!=fd2)
			FD_SET(fd2,&rd);

		int n = select(10,&rd,&wd,&xd,NULL);
		if(n<=0)
			break;

		if (FD_ISSET(fd1,&rd))
		{
			if(read(fd1,buf,BUFSIZ))
			{
			 FD_CLR(fd1,&rd);
			 sum+=atoi(buf);
			} else
				fd1=0;
		} else if(FD_ISSET(fd2,&rd))
		{
			if(read(fd2,buf,BUFSIZ))
			{
			 FD_CLR(fd2,&rd);
			 sum+=atoi(buf);
			} else
				fd2=0;
		}

		if(FD_ISSET(fd1,&xd) || FD_ISSET(fd2,&xd))
		{
			printf("!\n");
			break;
		}
	}

	printf("%d\n",sum);
	return 0;
}
