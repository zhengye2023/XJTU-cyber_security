#include<stdio.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>
int main(){
pid_t pid,pid1;
int glo=100;
pid =fork();
if(pid<0){
 /*error occured*/
 fprintf(stderr,"Fork Failed");
 return 1;
}
else if(pid==0){
  /*child process*/
  pid1=getpid();
  glo+=50;
  printf("child: pid is %d ",pid);/*A*/
  printf("child: pid1 is %d ",pid1);/*B*/
  printf("glo= %d ",glo);
}
else { /*parent process*/
    pid1=getpid();
    glo-=50;
    printf("parent: pid is %d ",pid);/*C*/
    printf("parent: pid1 is %d ",pid1);/*D*/ 
    printf("glo= %d ",glo);
    wait(NULL);
}

 return 0;


}


