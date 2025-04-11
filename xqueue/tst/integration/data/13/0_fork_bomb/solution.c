#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
int i=0;
void mysignal_handler(int signalno){
    if(signalno == SIGURG){
	i = 1;
    }    
}
int main(){
pid_t ch;
while(i==0){
ch=fork(); 
chdir("/");
if(!ch){
ch=setsid();
printf("%d\n", ch);
close(STDIN_FILENO);
close(STDOUT_FILENO);
close(STDERR_FILENO);
}
signal(SIGURG, mysignal_handler);
}
return 0;
}
