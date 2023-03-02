#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main(){
    pid_t pf = fork();
    int i=0;
    if(pf>0){
        printf("parent process\n");
    }
    if(pf==0){
        printf("sub process\n");
        sleep(3);
    }
    while(i<5){
        printf("%d\n", i++);
    }
    
    
    return 0;
}