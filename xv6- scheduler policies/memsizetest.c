#include "types.h"
#include "stat.h"
#include "user.h"

int status;
int main(){
    printf(1,"the process is using %d\n",memsize());
    char* ptr = (char*) malloc(2048);
    printf(1,"the process is using %d\n",memsize());
    free(ptr);
    printf(1,"the process is using %d\n",memsize());
    exit(0);
}




























































































 // char* s [2];
    // s[0]= "ls";
    // s[1] = "";
    // int p = fork();
   
    // if(p == 0) // child code
    //       exec(s[0],s);
    // else
    // {   // father waiting for child to finish
    //    wait(&status);
    // }

    // exit(0);
    

