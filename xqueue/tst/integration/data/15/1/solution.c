#include <stdio.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/shm.h>


int main(int c, char **v)
{
int i=0;
  if(c!=3)
    return printf("ERROR: call key1 key2i\n");


  int key1=atoi(v[1]);
  int key2=atoi(v[2]);

  int fd=0;
  char path[BUFSIZ]="/tmp/XXXXXXXXXXX";

  mkstemp(path);
  fd = open(path,O_CREAT);
  close(fd);
  key_t key3=ftok(path,1);


  int rr=shmget(key3,1000,IPC_CREAT|0666);
  int r1=shmget(key1,1000,IPC_CREAT|0666);
  int r2=shmget(key2,1000,IPC_CREAT|0666);
 
  int *ptr1 = shmat(r1,NULL,0);
  int *ptr2 = shmat(r2,NULL,0);
  int *ptr3 = shmat(rr,NULL,0);

//  printf("%p %p %p\n",ptr1,ptr2,ptr3);

  for(i=0;i<100;i++)
  {
    ptr3[i] = ptr1[i]+ptr2[i];  
  }
  
  printf("%d\n",key3);
  
  return 0;
}
