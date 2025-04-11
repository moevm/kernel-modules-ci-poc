#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

S
y
n
t
a
x

e
r
r



int get_ppid(int pid) 
{
	char buf[BUFSIZ], comm[BUFSIZ];
	int ppid,myppid=0,id;
	char state;

	sprintf(buf, "/proc/%d/stat",pid);

	int fd = open(buf,O_RDONLY);

	read(fd,buf,BUFSIZ);
	sscanf(buf, "%d %s %c %d",&id,comm,&state,&myppid);

	close(fd);
 
 	return myppid;
}

int is_intree(int root, int pid) 
{
	int current_pid = pid;

	//printf("test (%d,%d)\n",root,pid);

	do 
	{
		if(current_pid == root)
			return 1;
		current_pid = get_ppid(current_pid);	
	}	while ( current_pid >=1 );
		
	return 0;
}


int check_file(const char *name)
{
  char buf[BUFSIZ] = {0,};

  //printf("check_file:%s\n",name);

	int fd = open(name, O_RDONLY);
	if( -1 == fd)
		return 0;


  if(read(fd, buf, BUFSIZ)>0)
	{
		close(fd);
		return get_ppid(atoi(buf));
	}
 return -1;
}

int walk_procs(int root)
{
  char path[BUFSIZ];
	DIR *dir = opendir("/proc");
	int test_pid=0, count=0;
	struct dirent* de = readdir(dir);

	//printf("walk from [%d]\n",root);

	while(NULL !=de)
	{
		if(de->d_type & DT_DIR) 
		{
			sprintf(path,"/proc/%s/stat",de->d_name);
			test_pid = check_file(path);
			//printf("%d\n",test_pid);
			if(is_intree(root,test_pid))
				count++;
				//printf("%d\n",test_pid);
		}
		de = readdir(dir);
	}
	printf("%d\n", count);
}


int main(int c, char **v)
{
	if(c!=2)
		return -1;

	int pid = atoi(v[1]);
	walk_procs(pid);
}
