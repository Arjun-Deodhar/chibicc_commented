  .globl main
main:
  push %rbp
  mov %rsp, %rbp
  sub $0, %rsp
  mov $1, %rax
  cmp $0, %rax
  je  .L.else.1
  mov $1, %rax
  mov $2, %rax
  mov $3, %rax
  jmp .L.return
  jmp .L.end.1
.L.else.1:
  mov $4, %rax
  jmp .L.return
.L.end.1:
.L.return:
  mov %rbp, %rsp
  pop %rbp
  ret
