#include "types.h"
#include "stat.h"
#include "user.h"


int main(int argc, char** argv){
int i;
for(i=1; i < argc; i++){
    	if(strcmp(argv[i],"0") == 0){
            if(policy(0) == 0)
                printf(1,"Policy has been successfully changed to Default Policy\n");
            else
                printf(1,"hi\n");
        }
        else if(strcmp(argv[i],"1") == 0){
            if(policy(1) == 0)
                printf(1,"Policy has been successfully changed to Priority Policy\n");
            else
                printf(1,"hi\n");
        }
       else if(strcmp(argv[i],"2") == 0){
           if(policy(2) == 0)
                printf(1,"Policy has been successfully changed to CFS Policy\n");
            else
                printf(1,"hi\n");
        }
        else
        {
            printf(1,"Error replacing policy, no such a policy number (%s)\n",argv[i]);
        }
        
    }
    exit(0);

}