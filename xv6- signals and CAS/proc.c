 #include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}




int 
allocpid(void) 
{
  pushcli();
  int pid;
  do {
  pid = nextpid;
  }
  while ( !CAS(&nextpid , pid , pid+1) ) ;
  // acquire(&ptable.lock);         these were here
  // pid = nextpid++;
  // release(&ptable.lock);
  popcli();
  return nextpid;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  // acquire(&ptable.lock);

  pushcli();

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if( CAS (&p->state , UNUSED , EMBRYO) ){   //assuming if(CAS) is atomic            // doing cas to force the process to change it's state to embryo,
        goto found;                                        // and avoid other process enter the label (found)
    }
  }

  // release(&ptable.lock);
  popcli();
  return 0;

found:
// release(&ptable.lock);
  popcli();

  p->pid = allocpid();

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;


  // Leave room for backup trap frame and the sigcactions.
  // sp = p->kstack + sizeof (*p->backup_tf) + 32 * sizeof(struct sigaction);
  sp = p->kstack + 32 * sizeof(struct sigaction) + sizeof(*p->tf);
  sp -= sizeof *p->backup_tf;
  p->backup_tf = (struct trapframe*)sp; 

  for (int i = 0; i < 32; i++){
    sp -= sizeof (struct sigaction);
    p->actions[i] = (struct sigaction*)sp;
  }

  for(int i = 0 ; i < 32 ; i++){               // make all the handlers default
    p->actions[i]->sa_handler = SIG_DFL;
  }

  return p;
}


//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
    // acquire(&ptable.lock);
  pushcli();
  CAS(&p->state,EMBRYO,RUNNABLE) ;
  //  p->state = RUNNABLE;

  // release(&ptable.lock);
  popcli();
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  np->sigmask = curproc->sigmask;         // father handlers going to child

  for(int i = 0; i < 32; i++){
      np->actions[i]->sigmask = curproc->actions[i]->sigmask;
  }

  for(int i = 0; i < 32; i++){
      np->actions[i]->sa_handler = curproc->actions[i]->sa_handler;
  }

    // acquire(&ptable.lock);
  pushcli();
  CAS(&np->state,EMBRYO,RUNNABLE);
  //  p->state = RUNNABLE;

  // release(&ptable.lock);
  popcli();

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  //acquire(&ptable.lock);
  pushcli();

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

   // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE || p->state == -ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  CAS(&curproc->state,RUNNING,-ZOMBIE);
  // curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  volatile int x;
  struct proc *curproc = myproc();
  
  //acquire(&ptable.lock);
  pushcli();
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)  // currproc different for diffrent cpu so no race condition
        continue;
      havekids = 1;
      if(p->state == ZOMBIE || p->state == -ZOMBIE){
        do{
          x = p->state;
        }
        while (! (CAS(&x,ZOMBIE,UNUSED)));
        CAS(&p->state, ZOMBIE, UNUSED);
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        //p->state = UNUSED;
        //release(&ptable.lock);
        popcli();
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      popcli();
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
     //acquire(&ptable.lock);
    pushcli();
     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){       
      if(CAS(&p->state,RUNNABLE,RUNNING)  ){
        
      // if(p->state != RUNNABLE)
      //   continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      // p->state = RUNNING;
    
      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
      CAS(&p->state,-RUNNABLE,RUNNABLE);    // for the problem in sched when yeilding
      CAS(&p->state,-SLEEPING,SLEEPING);    // for the problem in sched when sleeping
      CAS(&p->state,-ZOMBIE,ZOMBIE);        // for the problem in sched when going zombie
      }
      
    }

    // release(&ptable.lock);
    popcli();

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  // if(!holding(&ptable.lock))
  //   panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  
 //cpu1 stop, cpu2 choose p 
  swtch(&p->context, mycpu()->scheduler);

  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void){
  //acquire(&ptable.lock);        //DOC: yieldlockcli
  pushcli();
  //myproc()->state = RUNNABLE;
  CAS(&myproc()->state,RUNNING,-RUNNABLE);
  sched();
  // release(&ptable.lock);
  popcli();
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void){
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  //release(&ptable.lock);
  popcli();

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  // if(lk == 0)
  //   panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.

  if(lk != &ptable.lock){  //DOC: sleeplock0
    //acquire(&ptable.lock);  //DOC: sleeplock1
    pushcli();
    release(lk);
  }
  // Go to sleep.

  CAS(&myproc()->state,RUNNING,-SLEEPING);

  p->chan = chan;
  
  // p->state = SLEEPING; //was here at the beginning

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  
  if(lk != &ptable.lock){  //DOC: sleeplock2
    //release(&ptable.lock);
    popcli();
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan){
  struct proc *p;
  volatile int x;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if( p->chan == chan ){
      if(p->state == -SLEEPING || p->state == SLEEPING){
        do{
            x = p->state;
        }
        while (!CAS(&x,SLEEPING, RUNNABLE));
      }
      CAS(&p->state, SLEEPING, RUNNABLE);
    }
  }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan){
  // acquire(&ptasble.lock);
  pushcli();
  wakeup1(chan);
  popcli();
  // release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid, int signum)
{
  struct proc *p;
  volatile int x;

  // acquire(&ptable.lock);
  pushcli();
  if(signum < 0 || signum > 31){
    popcli();
    return -1;
  }
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      uint a = 1;
      a = a<<31;            // 1 and 31 zeroes
      a = a>>signum;        // shift right
      p->PS = p->PS | a;

       if(p->state == -SLEEPING || p->state == SLEEPING){
        do{
          x = p->state;
        }
        while (!CAS(&x,SLEEPING,RUNNABLE));
        CAS(&p->state, SLEEPING, RUNNABLE);
      }

      popcli();
      return 0;
    }
  }
  popcli();
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
