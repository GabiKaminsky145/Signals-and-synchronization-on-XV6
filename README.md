# signals-and-synchronization-on-XV6


This version of XV6 includes an implementation of processes synchronization using CAS, also it includes handling system signals and user signals.

In the first part, we add a signal mask of 32 bit to every process, whenever a signal is called by the system (kill, stop or continue) or the user, the program checks the signal mask and acts properly. The action of the signal is a function handler as part of the signal object that we created.
When the handler is called, we have used the mechanism of calling a function in Assembly (push arguments, return address, etc.), this enables the program to return to its original call before the signal handler interrupts. 

In the second part of the assignment, we had the support of Compare and Swap mechanism, with this mechanism we can let multiple processes work simultaneously compared to the naive XV6.
The CAS mechanism can be mostly viewed on the proc.c file. 
To fully synchronized the system, we needed to consider the state of every process,  therefore we notice that using primitive states like Running, Ready, Sleep, isn't enough because every process is doing a lot of work every step. This problem was solved by creating new states in the process header.
