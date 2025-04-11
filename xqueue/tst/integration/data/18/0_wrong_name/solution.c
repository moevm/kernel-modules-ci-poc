#include <stdio.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

struct sockaddr_in local;	

int compare(const void *a, const void *b)
{
	const char *pa = (const char *)a;
	const char *pb = (const char *)b;

	return *pa < *pb;
}


int server(int port)
{
		int ss=socket (AF_INET,SOCK_STREAM,0);
		int cs;
		char buf[BUFSIZ];
		
		inet_aton("127.0.0.1",&local.sin_addr);
		local.sin_port = htons(port);
		local.sin_family= AF_INET;
		int r = bind(ss, (struct sockaddr*)   &local, sizeof(local));

		r=listen(ss,5);
		
		cs=accept(ss,NULL,NULL); 

		int len=0;
		while(len=read(cs,buf,BUFSIZ))
		{

			if( 0== strncmp("OFF",buf,3))
			{
				close(ss);
				close(cs);
				exit(0);
			}
			
			qsort(buf,len-1,1,compare);
			write(cs,buf,len);
		}
}


int main(int c, char **v)
{
	if(c!=2)
	{
		return printf("Use: %s port\n",v[0]);
	}

	server(atoi(v[1]));
}



