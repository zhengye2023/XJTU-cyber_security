#include <stdio.h>
#include <unistd.h>
int main(){
  pid_t pid=getpid();
  pid_t ppid=getppid();
  printf("system_call: pid=%d, ppid=%d\n",pid,ppid);
  return 0;
}  