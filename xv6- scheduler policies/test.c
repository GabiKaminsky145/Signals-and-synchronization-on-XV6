#include "types.h"
#include "stat.h"
#include "user.h"
#include "perf.h"

#define FIB_SLEEP 1
#define FIB_NO_SLEEP 2

int fib(int n);
void fib_with_sleep();
void fib_no_sleep();
void print_proc_info();
void run_test (int test_num);

void test_ps_sys_call();
void test_cfs_sys_call();

void test_ps();
void test_ps_fork1();
void test_ps_fork2();

void test_cfs();
void test_cfs_fork();


int main(int argc, char *argv[])
{   
    /*** system calls tests ***/
    run_test(1);
    run_test(2);

    /*** ps tests ***/
    run_test(3);
    run_test(4);
    run_test(5);

    /*** cfs tests ***/
    run_test(6);
    run_test(7);

    exit(0);
}

void run_test (int test_num) {

    printf(1, "****************** Test Num: %d ******************\n", test_num);
    switch (test_num) {
        case 1:
            test_ps_sys_call();
            break;
        case 2:
            test_cfs_sys_call();
            break;
        case 3:
            policy(1);
            test_ps();
            break;
        case 4:
            policy(1);
            test_ps_fork1();
            break;
        case 5:
            policy(1);
            test_ps_fork2();
            break;
        case 6:
            policy(2);
            test_cfs();
            break;
        case 7:
            policy(2);
            test_cfs_fork();
            break;
    }
    printf(1, "****************** Finished, please check yourself ******************\n \n");
}

// checks set_ps_priority
void test_ps_sys_call() {
    int success = 1;
    for (int i=1; i<=10; i++) {
        if (set_ps_priority(i) != 0) {
            printf(1, "BUG: set_ps_priority\n");
            success=0;
        }
    }

    if (set_ps_priority(-1) != -1) {
        printf(1, "BUG: set_ps_priority- not returning error\n");
        success=0;
    }

    if (set_ps_priority(11) != -1) {
        printf(1, "BUG: set_ps_priority- not returning error\n");
        success=0;
    }

    if (success == 1) {
        printf(1, "test of set_ps_priority - SUCCESS\n");
    }
}

// checks set_cfs_priority
void test_cfs_sys_call() {
    int success = 1;
    for (int i=1; i<=3; i++) {
        if (set_cfs_priority(i) != 0) {
            printf(1, "BUG: set_cfs_priority\n");
            success=0;
        }
    }

    if (set_cfs_priority(4) != -1) {
        printf(1, "BUG: set_cfs_priority- not returning error\n");
        success=0;
    }

    if (set_cfs_priority(-1) != -1) {
        printf(1, "BUG: set_cfs_priority- not returning error\n");
        success=0;
    }

    if (success == 1) {
        printf(1, "test of set_cfs_priority - SUCCESS\n");
    }
}

// executes the tested procs:
// 1. sets its priority
// 2. calls the wanted fib version
// 3. prints its statistics
// 4. exits
void ps_proc_task(int priority, int fib_type) {
    
    if (priority>0) {
        set_ps_priority(priority);
    }

    switch (fib_type) {
        case FIB_SLEEP: 
            fib_with_sleep();
            break;
        case FIB_NO_SLEEP:
            fib_no_sleep();
            break;
    }
            
    print_proc_info();
    exit(0);
}

/* ps test:
*
* creates 6 procs- all of them calculate fib(40) 3 times.
* 2 procs- high priority = 1.
* 2 procs - mid priority = 5.
* 2 procs- low priority = 10. 
* for each pair- one of them goes to sleep between each fib iteration and the second isn't.
* you need to verify: 
* each pair has the same retime.
* the retime of the high > retime of mid ones > retime of the low ones.
* the rtime of all 6 procs is same.
*/
void test_ps () {

    if (fork() == 0) {
        int pid = getpid();
        printf(1, "proc num: %d -> high+sleeping\n", pid);
        ps_proc_task(1, FIB_SLEEP);
    }

    if (fork() == 0) {
        int pid = getpid();
        printf(1, "proc num: %d -> high+not sleeping\n", pid);
        ps_proc_task(1, FIB_NO_SLEEP);
    }

    if (fork() == 0) {
        int pid = getpid();
        printf(1, "proc num: %d -> mid+sleeping\n", pid);
        ps_proc_task(5, FIB_SLEEP);
    }

    if (fork() == 0) {
        int pid = getpid();
        printf(1, "proc num: %d -> mid+not sleeping\n", pid);
        ps_proc_task(5, FIB_NO_SLEEP);
    }

    if (fork() == 0) {
        int pid = getpid();
        printf(1, "proc num: %d -> low+sleeping\n", pid);
        ps_proc_task(10, FIB_SLEEP);
    }

    if (fork() == 0) {
        int pid = getpid();
        printf(1, "proc num: %d -> low+not sleeping\n", pid);
        ps_proc_task(10, FIB_NO_SLEEP);
    }

    for (int i=1; i<=6; i++) {
        wait(null);
    }
}

// executes the tested procs:
// 1. sets its priority
// 2. calls the wanted fib version
// 3. prints its statistics
// 4. exits
void ps_parent_task(int fib_type) {
    switch (fib_type) {
        case FIB_SLEEP: 
            fib_with_sleep();
            break;
        case FIB_NO_SLEEP:
            fib_no_sleep();
            break;
    }
    print_proc_info();
    wait(null);
    exit(0);
}

// ps fork test:
/* 
* test that doing fork makes the child proccess priority as default- 5 .
* 2 procs: 1 high, 1 low.
* each performs fork.
* both children should have mid priority.
*/
void test_ps_fork1() {
    if (fork() == 0) {
        set_ps_priority(1);

        int pid = getpid();
        printf(1, "PARENT: %d -> high\n", pid);
        
        if (fork()==0) {
            int pid = getpid();
            printf(1, "CHILD: %d -> mid\n", pid);
            ps_proc_task(-1, FIB_NO_SLEEP);
        }

        ps_parent_task(FIB_NO_SLEEP);
    }

    if (fork() == 0) {
        set_ps_priority(10);
        int pid = getpid();
        printf(1, "PARENT: %d -> low\n", pid);

        if (fork()==0) {
            int pid = getpid();
            printf(1, "CHILD: %d -> mid\n", pid);
            ps_proc_task(-1, FIB_NO_SLEEP);
        }

        ps_parent_task(FIB_NO_SLEEP);
    }

    wait(null);
    wait(null);
}

// ps fork test:
/* 
* test that doing fork makes the child proccess priority as default- 5 .
* 2 procs: 1 high, 1 low.
* each performs fork.
* each child sets its priority to be like its parent.
* both children should behave like their parents.
*/
void test_ps_fork2() {
    if (fork() == 0) {
        set_ps_priority(1);

        int pid = getpid();
        printf(1, "PARENT: %d -> high\n", pid);
        
        if (fork()==0) {
            int pid = getpid();
            printf(1, "CHILD: %d -> high\n", pid);
            ps_proc_task(1, FIB_NO_SLEEP);
        }

        ps_parent_task(FIB_NO_SLEEP);
    }

    if (fork() == 0) {
        set_ps_priority(10);
        int pid = getpid();
        printf(1, "PARENT: %d -> low\n", pid);

        if (fork()==0) {
            int pid = getpid();
            printf(1, "CHILD: %d -> low\n", pid);
            ps_proc_task(10, FIB_NO_SLEEP);
        }

        ps_parent_task(FIB_NO_SLEEP);
    }

    wait(null);
    wait(null);
}

// proc task code:
// 1. sets its priority
// 2. calls the wanted fib version
// 3. prints its statistics
// 4. exits
void cfs_proc_task (int priority, int fib_type) {
    
    if (priority>0) {
        set_cfs_priority(priority);
    }

    switch (fib_type) {
        case FIB_SLEEP: 
            fib_with_sleep();
            break;
        case FIB_NO_SLEEP:
            fib_no_sleep();
            break;
    }
            
    print_proc_info();
    exit(0);
}

// cfs test:
/*
* creates 6 procs- all of them calculate fib(40) 3 times.
* 2 procs- high priority.
* 2 procs - mid priority.
* 2 procs- low priority.  
* for each pair- one of them goes to sleep between each fib iteration and the second isn't.
* you need to verify: 
* each pair has the same retime.
* the retime of the high > retime of mid ones > retime of the low ones.
* the rtime of all 6 procs is same.
*/
void test_cfs() {

    /*** high priority ***/
    if (fork() == 0) {
        int pid = getpid();
        printf(1, "proc num: %d -> high+sleeping\n", pid);
        cfs_proc_task(1, FIB_SLEEP);
    }

    if (fork() == 0) {
        int pid = getpid();
        printf(1, "proc num: %d -> high+not sleeping\n", pid);
        cfs_proc_task(1, FIB_NO_SLEEP);
    }

    /*** normal priority ***/
    if (fork() == 0) {
        int pid = getpid();
        printf(1, "proc num: %d -> normal+sleeping\n", pid);
        cfs_proc_task(2, FIB_SLEEP);
    }

    if (fork() == 0) {
        int pid = getpid();
        printf(1, "proc num: %d -> normal+not sleeping\n", pid);
        cfs_proc_task(2, FIB_NO_SLEEP);
    }

    /*** low priority ***/
    if (fork() == 0) {
        int pid = getpid();
        printf(1, "proc num: %d -> low+sleeping\n", pid);
        cfs_proc_task(3, FIB_SLEEP);
    }

    if (fork() == 0) {
        int pid = getpid();
        printf(1, "proc num: %d -> low+not sleeping\n", pid);
        cfs_proc_task(3, FIB_NO_SLEEP);
    }

    // main proc waits that the tested procs will finish
    for (int i=1; i<=6; i++) {
        wait(null);
    }
}

// parent task code:
// 1. calls the wanted fib version
// 2. prints its statistics
// 3. waits for child
// 4. exits
void cfs_parent_task(int fib_type) {
    switch (fib_type) {
        case FIB_SLEEP: 
            fib_with_sleep();
            break;
        case FIB_NO_SLEEP:
            fib_no_sleep();
            break;
    }
    print_proc_info();
    wait(null);
    exit(0);
}

// cfs fork test:
/*
* creates 4 procs- all of them calculate fib(40) 3 times.
* 2 procs- high priority.
* 2 procs- low priority.  
* for each pair- one of them goes to sleep between each fib iteration and the second isn't.
* each proc creates child, that suppose to act like it.
* you need to verify: 
* all 4 procs (2 parents+2 children) with same priority should have same retime.
* the retime of the high > retime of the low ones.
* the rtime of all 6 procs is same.
*/
void test_cfs_fork() {
    if (fork() == 0) {
        set_cfs_priority(1);
        int pid = getpid();
        printf(1, "PARENT: %d -> high+sleeping\n", pid);
        
            if (fork()==0) {
                int pid = getpid();
                printf(1, "CHILD: %d -> high+sleeping\n", pid);
                cfs_proc_task(-1, FIB_SLEEP);
            }
        cfs_parent_task(FIB_SLEEP);
    }

    if (fork() == 0) {
        set_cfs_priority(1);
        int pid = getpid();
        printf(1, "PARENT: %d -> high+not sleeping\n", pid);

        if (fork()==0) {
                int pid = getpid();
                printf(1, "CHILD: %d -> high+not sleeping\n", pid);
                cfs_proc_task(-1, FIB_NO_SLEEP);
            }

        cfs_parent_task(FIB_NO_SLEEP);
    }

    if (fork() == 0) {
        set_cfs_priority(3);
        int pid = getpid();
        printf(1, "PARENT: %d -> low+not sleeping\n", pid);

        if (fork()==0) {
                int pid = getpid();
                printf(1, "CHILD: %d -> low+sleeping\n", pid);
                cfs_proc_task(-1, FIB_SLEEP);
            }

        cfs_parent_task(FIB_SLEEP);
    }

    if (fork() == 0) {
        set_cfs_priority(3);
        int pid = getpid();
        printf(1, "proc num: %d -> low+not sleeping\n", pid);

        if (fork()==0) {
                int pid = getpid();
                printf(1, "CHILD: %d -> low+not sleeping\n", pid);
                cfs_proc_task(-1, FIB_NO_SLEEP);
            }

        cfs_parent_task(FIB_NO_SLEEP);
    }

    for (int i=1; i<=4; i++) {
        wait(null);
    }
}

int fib(int n) 
{
    if (n==1 || n==0) {
        return 1;
    }

    return fib(n-1) + fib(n-2);
}

void fib_with_sleep() {
    for (int i=0; i<3; i++) {
            fib(40);
            sleep(10);
        }
}

void fib_no_sleep() {
    for (int i=0; i<3; i++) {
            fib(40);
        }
}

void print_proc_info(){
    int pid = getpid();
    struct perf proc_perf;
        // get statistics and print
        proc_info(&proc_perf);

        printf(1, "pid: %d, ps_priority: %d, stime: %d, retime: %d, rtime: %d\n", 
                    pid, proc_perf.ps_priority, proc_perf.stime, proc_perf.retime, proc_perf.rtime);
}