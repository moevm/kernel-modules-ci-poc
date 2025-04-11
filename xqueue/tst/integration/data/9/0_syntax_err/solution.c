#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

23212312
dfdsfsdfsd

int main(int c, char **v)
{

  pid_t p = fork();

  if(p ==0)
  {
    setsid();
    p = getpid();
    printf("%d\n", p);
    while(1) usleep(500000);
  } else
  {
    return 0;    
  }
  return 0;
}

