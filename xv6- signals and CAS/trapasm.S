#include "mmu.h"
#include "syscall.h"
#include "traps.h"

  # vectors.S sends all traps here.
.globl alltraps
alltraps:
  # Build trap frame.
  pushl %ds
  pushl %es
  pushl %fs
  pushl %gs
  pushal
  
  # Set up data segments.
  movw $(SEG_KDATA<<3), %ax
  movw %ax, %ds
  movw %ax, %es

  # Call trap(tf), where tf=%esp
  pushl %esp
  call trap
  addl $4, %esp

  # Return falls through to trapret...
.globl trapret
trapret:
# do stuff in assembly to check for signals in the PS field

pushl %esp
  call handle
 addl $4, %esp

# this was here before:
  popal
  popl %gs
  popl %fs
  popl %es
  popl %ds
  addl $0x8, %esp  # trapno and errcode
  iret
# for address math(see handle_user() in trap.c)
.globl start_sigret
.globl end_sigret
  start_sigret:
  movl $SYS_sigret,%eax
  int $T_SYSCALL

  ; call sys_sigret
  ; iret
  end_sigret:
  
  