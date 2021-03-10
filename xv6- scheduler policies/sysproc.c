#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "perf.h"
#include "spinlock.h"

struct perf* performence;
int sched_type;
struct
{
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;


// struct perf {
// int ps_priority;
// int stime;    //sleeping time
// int retime;     //ready time
// int rtime;      //running time
// };


int sys_fork(void)
{
  return fork();
}

void sys_exit(int status)
{
  exit(0);
}

int sys_wait(int *status)
{
  return wait(status);
}

int sys_kill(void)
{
  int pid;

  if (argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int sys_getpid(void)
{
  return myproc()->pid;
}

int sys_sbrk(void)
{
  int addr;
  int n;

  if (argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

int sys_sleep(void)
{
  int n;
  uint ticks0;

  if (argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (myproc()->killed)
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int memsize(void)
{

  return myproc()->sz;
}

int set_ps_priority(int priority)
{
  if (priority > 10 || priority < 0)
  {
    return -1;
  }

  myproc()->ps_priority = priority;
  return 0;
}

int sys_set_ps_priority(void)
{
  int priorty;

  if (argint(0, &priorty) < 0)
    return -1;
  return set_ps_priority(priorty);
}

int set_cfs_priority(int priority)
{
  if (priority > 3 || priority < 1)
  {
    return -1;
  }
  if (priority == 1)
    myproc()->decay_factor = 0.75;
  else if (priority == 2)
    myproc()->decay_factor = 1;
  else
    myproc()->decay_factor = 1.25;
  return 0;
}

int sys_set_cfs_priority(void)
{
  int priorty;

  if (argint(0, &priorty) < 0)
    return -1;
  return set_cfs_priority(priorty);
}


int policy(int policy)
{
  if (policy > 2 || policy < 0)
  {
    return -1;
  }
  sched_type = policy;
  return 0;
}

int sys_policy(void)
{
  int policy_arg;

  if (argint(0, &policy_arg) < 0)
    return -1;
  return policy(policy_arg);
}

int proc_info(struct perf * performance){
  acquire(&ptable.lock);
  struct proc * p = myproc();
  performance->stime = p->stime;
  performance->rtime = p->rtime;
  performance->retime = p->retime;
  performance->ps_priority = p->ps_priority;
  cprintf("%d        %d          %d      %d      %d        \n",p->pid, performance->ps_priority, performance->stime,performance->retime,performance->rtime);
  release(&ptable.lock);
  return 0;
}

int sys_proc_info(void){
 struct perf *performance; 
  if(argptr(0, (void*)&performance, sizeof(*performance)) < 0)
    return -1; 

return proc_info(performence);

}