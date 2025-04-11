#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <fcntl.h>


int count = 0;

void check_file(const char *name)
{
  char buf[BUFSIZ] = {0,};
	int fd = open(name, O_RDONLY);
	if( -1 == fd)
		return;

	read(fd, buf, BUFSIZ);



	if(!strncmp("genenv",buf,6))
  {
		count++;
	//  printf("%s [%s]\n",name,buf);
	}
	return;

  close(fd);
}


int count_procs()
{
  char path[BUFSIZ];
	DIR *dir = opendir("/proc");
	struct dirent* de = readdir(dir);

	while(NULL !=de)
	{
		if(de->d_type & DT_DIR) 
		{
			sprintf(path,"/proc/%s/comm",de->d_name);
			check_file(path);
		}
		de = readdir(dir);
	}

}

int main()
{
	count_procs();
	printf("%d\n",count);
}

