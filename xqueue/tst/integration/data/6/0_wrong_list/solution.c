#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

int main(int c, char **v)
{
  if(c!=2)
    return -1;

  int pid = atoi(v[1]);
  int i = 0;
  for (i= 222; i < 229; i++)
  {
    printf("%d\n", i);
  }
}

