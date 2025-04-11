#include <stdio.h>
#include <strings.h>


Hello Sir, I am a syntax error!

long count =0;


void count_sym(char *str)
{

	while(*str!='\0')
	{
		if( *str == '0' )
			count++;
		str++;	
	}
}


int main(int c, char **v)
{
	char buf[BUFSIZ];

	if(c!=3)
		return printf("ERROR: call %s filename seed",v[0]);

	sprintf(buf,"./%s %s",v[1],v[2]);
	
	
	FILE *output = popen(buf,"r");

	if(NULL == output ) 
	{

		perror("File not open\n");
		return -1;
	}

	while(NULL!=fgets(buf,BUFSIZ,output))
	{
		count_sym(buf);
	}

	printf("%ld\n",count);
	return 0;
}
