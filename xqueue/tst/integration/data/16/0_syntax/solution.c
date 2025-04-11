#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>



even domain names may contain errors

int main(int c, char **v)
{
	struct hostent *h;


	if(NULL == (h=gethostbyname(v[1])))
	{
		perror("hostbyname");
		return 0;
	}
	
	printf("%s, type=%d, len=%d a=%p\n",h->h_name,h->h_addrtype,h->h_length,h->h_aliases[0]);
	int i=0;

	while(h->h_addr_list[i]!=NULL)
	{
		struct in_addr *a = (struct in_addr*)  h->h_addr_list[i];
		printf("%d: %s\n",i,inet_ntoa( *a));
		i++;
	}

// kkv: reserver for exam
//	i=0;
//	while(h->h_aliases[i])
//	{
//		printf("%s\n",h->h_aliases[i++]);
//	}


}
