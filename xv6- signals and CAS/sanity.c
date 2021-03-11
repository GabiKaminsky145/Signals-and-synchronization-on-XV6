#include "types.h"
#include "stat.h"
#include "user.h"

int count1 = 0;
int sigmask1 = 0;
void handler1(int signum){
    count1++;
   
}


int main(){
    // struct sigaction sa;
    // memset(&sa,0,sizeof(sa));
    // sa.sa_handler = &handler1;
    // sa.sigmask = sigmask1;
    // sigaction(8,&sa,null);
    int pid = fork();
    int pid2 = fork();
    int i;
    if(pid ==0){
        for(i = 0;  i < 1000; i++){
            printf(1,"first child %d ",i);
            printf(1,"");
        }
        exit();
    }
    if(pid2 == 0){
        for (int i = 0; i < 1000; i++){
             printf(1,"second child %d ",i);
        }
    }
    kill(pid,17); //stop
    printf(1,"sent sigstop to child\n");
    sleep(5000);
    kill(pid,19); //cont
    wait();
    wait();
    exit();
}
