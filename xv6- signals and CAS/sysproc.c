#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

uint sigprocmask(uint sigprocmask);
int sigaction(int signum,const struct sigaction * act,struct sigaction * oldact);

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;
  int signum;
  if(argint(0, &pid) < 0)
    return -1;
  if(argint(1, &signum) < 0){
    return -1;
  }
  return kill(pid,signum);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
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
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int sys_sigprocmask(void){
  int procmask;
  if(argint(0, &procmask) < 0)
    return -1;
  return sigprocmask(procmask);
}

uint sigprocmask(uint sigprocmask){
  uint oldmask = myproc()->sigmask;
  myproc()->sigmask = sigprocmask;
  return oldmask;
}


int sys_sigaction(void){
  int signum;
  if(argint(0, &signum) < 0)
    return -1;
  struct sigaction * act;
  if (argptr(1, (void*)&act, sizeof(*act)) < 0)
    return -1;
  struct sigaction * oldact;
  if (argptr(2, (void*)&oldact, sizeof(*oldact)) < 0){
    return -1;
  }
    return sigaction(signum,act,oldact);
}

int sigaction(int signum,const struct sigaction * act,struct sigaction * oldact){
  struct proc* p = myproc();
  if(signum == SIGKILL || signum == SIGSTOP)
    return -1;
  if(oldact != null)
    oldact->sa_handler = p->actions[signum]->sa_handler;
  if(act != null){
    p->actions[signum]->sa_handler = act->sa_handler;         // update the fields of the specific action in action array of the process
    p->actions[signum]->sigmask = act->sigmask;
  }
  return 0;
}

int sigret(){
  if (myproc() == 0)
    return 0;
  struct proc * p = myproc();
  p->sigmask = p->maskbackup;
  memmove(p->tf, p->backup_tf, sizeof(struct trapframe));
  // *p->tf = *p->backup_tf;                                     // same code from fork()
  return 1;
}

int sys_sigret(void){
  return sigret();
}


