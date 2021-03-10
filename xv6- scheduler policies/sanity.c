#include "types.h"
#include "stat.h"
#include "user.h"


int p1, p2, p3;
int status;
// struct perf *curr;

struct perf {
int ps_priority;
int stime;
int retime;
int rtime;
};

int main(int argc, char **argv)
{
   struct perf* curr1 = malloc(sizeof(*curr1));
    struct perf* curr2 = malloc(sizeof(*curr2));
     struct perf* curr3 = malloc(sizeof(*curr3));

    printf(1,"PID    PS_PRIORITY   STIME  RETIME   RTIME\n");
    p1 = fork();
    if (p1 == 0) // first child
    {
        set_ps_priority(10);
        set_cfs_priority(1.25);
        long long i = 10000000;
        double dummy = 0;
        while (i--)
        {
            dummy += i;
             printf(1,"");
        
        }
        proc_info(curr1);
        free(curr1);
        exit(0);
    }
    else
    {
        p2 = fork();
        if (p2 == 0)
        { //second child
            set_ps_priority(5);
            set_cfs_priority(1);
            long long i = 10000000;
            double dummy = 0;
            while (i--)
            {
                dummy += i;
                printf(1,"");
              
            }
            proc_info(curr2);
            free(curr2);
            exit(0);
        }
        else
        {
            p3 = fork();
            if (p3 == 0)
            {
                //third child
                set_ps_priority(1);
                set_cfs_priority(0.75);
                long long i = 10000000;
                double dummy = 0;
                while (i--)
                {
                    dummy += i;
                     printf(1,"");
                    
                }
                proc_info(curr3);
                free(curr3);
                exit(0);
            }
            else
            {   //parent code
                wait(&status);
                wait(&status);
                wait(&status);
                exit(0);
            }
        }
    }
    exit(0);
}