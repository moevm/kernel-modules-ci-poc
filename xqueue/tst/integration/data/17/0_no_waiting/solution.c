#include <stdio.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdlib.h>


const size_t BUFS = 2*32768;


int main(int c, char **v)
{
	char buf[BUFS];
	if(c!=2)
	{
		return printf("Use: %s port\n",v[0]);
	}

	struct sockaddr_in local;	

		int s=socket (AF_INET,SOCK_DGRAM,0);
		
		inet_aton("127.0.0.1",&local.sin_addr);
		local.sin_port = htons(atoi(v[1]));
		local.sin_family= AF_INET;
		bind(s, (struct sockaddr*)   &local, sizeof(local));
	
}



