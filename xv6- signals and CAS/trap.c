#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#define ALLONES (2^32) - 1

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[]; // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

struct
{
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

extern void start_sigret(void);
extern void end_sigret(void);

void handle_user(int signum);

void tvinit(void)
{
  int i;

  for (i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE << 3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE << 3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void idtinit(void)
{
  lidt(idt, sizeof(idt));
}

void sigcont(struct proc *p)
{
  uint a = 1<<31;   // shift to turn on the CONT bit
  a = a >> SIGCONT; 
  p->PS = p->PS | a;
  p->channel = 0;
}

void sigstop(struct proc *p)
{
  acquire(&ptable.lock);
  p->state = RUNNABLE;
  release(&ptable.lock);
  p->channel = SIGCONT;
  uint a = (ALLONES) - (1 << SIGCONT); // turn off the CONT bit (to prevent a bug when we stop/cont more than once)
  p->PS = p->PS & a;



  //pending in location i  need to be 0
}

void sigkill(struct proc *p)
{
  p->PS = 0;
  p->killed = 1;
  acquire(&ptable.lock);
  if (p->state == SLEEPING) // wake process from sleep if nessecery
    p->state = RUNNABLE;
  release(&ptable.lock);
  return;
}

int bitoff(uint arr, uint location)
{
  // 0101
  arr = arr >> location;
  return !(arr & 1); // if the bit is off return 1
}

void debug(){
  // cprintf("debug\n");
  return;
}

void handle(struct trapframe* tf){
  if((tf->cs & 3) != DPL_USER){
    return;
  }
  struct proc * p = myproc();
  if(p == 0 || p->PS == 0){
    // cprintf("null proc\n");
    return;
  }                                                                     // 000100001110000000000000
  uint check = 1<<31;                                                   // 100000000000000000000000
  for (int i = 0; i < 32; i++){
    if((check & p->PS) > 0){
      if (i == SIGKILL){
        sigkill(p);
        return;
      }
      if (i == SIGSTOP){
        sigstop(p);
        goto next;
      }
      if ((uint)p->actions[i]->sa_handler == SIG_IGN){
        goto next;
      }
      if (p->actions[i]->sa_handler == SIG_DFL && i != SIGCONT){ //we got here so i != SIGSTOP
          sigkill(p);
          goto next;
      }
      else if ((i == SIGCONT) && (bitoff(p->sigmask,i))){
        sigcont(p);
        goto next;
      }
      else{
      //not a kernel signal
      if(bitoff(p->sigmask,i)){
        handle_user(i);
       }
     }
    }
    next:
    check = check>>1;
  }
}



void handle_user(int signum){
  struct proc *p = myproc();
  memmove(p->backup_tf, p->tf, sizeof(struct trapframe));
  // *p->backup_tf = *p->tf;                            // this line is the same as the line in fork. maybe we should use memmove
  p->sigmask = p->actions[signum]->sigmask;
  uint size = (uint)&end_sigret - (uint)&start_sigret;
  p->tf->esp -= size;                                   // leave room in stack for the sigret label
  memmove(&p->tf->esp, &start_sigret, size);            // esp point to start_sigret
  p->tf->esp -= 4;                                      // leave space for signal number
  *((int *)p->tf->esp) = signum;                        // the argument of the function
  p->tf->eip = (int)p->actions[signum]->sa_handler;     // put handler function in eip
}


// void handle_user(int signum,struct proc* p){
//       memmove(&p->backup, p->tf, sizeof(struct trapframe));
//       // *p->backup = *p->tf;                                       // this line is the same as the line in fork. maybe we should use memmove
//        p->sigmask = p->actions[signum].sigmask;
//        uint size = (uint)&end_sigret - (uint)&start_sigret;
//        p->tf->esp -= size;
//        memmove(&p->tf->esp, start_sigret, size);   ///
//               // p->tf->esp -= 4;
//               // *((uint *)p->tf->esp) = i;                                  // argument of handler
//               // p->tf->esp -= 4;
//               // p->tf->esp = (uint)&p->tf->esp + 8;
//         *((uint *)(p->tf->esp - 4)) = signum;  
//         *((uint *)(p->tf->esp - 8)) = p->tf->esp;
//         p->tf->esp -= 8 ;                                              // put the return address to return to start_sigret(memmove works with address)
//         *((uint *)p->tf->eip) = (uint)p->actions[signum].sa_handler;        // put handler function in eip
// }


//PAGEBREAK: 41
void trap(struct trapframe *tf)
{
  // struct proc* p = myproc();
  if (tf->trapno == T_SYSCALL)
  {
    if (myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if (myproc()->killed)
      exit();
    return;
  }

  switch (tf->trapno)
  {
  case T_IRQ0 + IRQ_TIMER:
    if (cpuid() == 0)
    {
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE + 1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  //PAGEBREAK: 13
  default:
    if (myproc() == 0 || (tf->cs & 3) == 0)
    {
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if (myproc() && myproc()->killed && (tf->cs & 3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if (myproc() && myproc()->state == RUNNING &&
      tf->trapno == T_IRQ0 + IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if (myproc() && myproc()->killed && (tf->cs & 3) == DPL_USER)
    exit();
}
