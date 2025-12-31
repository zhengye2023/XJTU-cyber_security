#include<stdio.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>
int main(){
pid_t pid,pid1;
pid =fork();
if(pid<0){
 /*error occured*/
 fprintf(stderr,"Fork Failed");
 return 1;
}
else if(pid==0){
  /*child process*/
  pid1=getpid();
  printf("child: pid is %d",pid);/*A*/
  printf("child: pid1 is %d",pid1);/*B*/
}
else { /*parent process*/
    pid1=getpid();
    printf("parent: pid is %d",pid);/*C*/
    printf("parent: pid1 is %d",pid1);/*D*/ 
    wait(NULL);
}
 return 0;
}



