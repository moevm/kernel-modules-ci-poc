#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <stdlib.h>


void term(int s)
{
  exit(0);
}


int main(int c, char **v)
{

  pid_t p = fork();

  if(p ==0)
  {
    setsid();
    p = getpid();
    printf("%d\n", p);
    signal(SIGURG,term);

    usleep(50000);
  } else
  {
    return 0;    
  }
  return 0;
}

