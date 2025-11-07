  .globl main
main:
  push %rbp
  mov %rsp, %rbp
  sub $32, %rsp
  mov $8, %rax
  push %rax
  mov $1, %rax
  pop %rdi
  imul %rdi, %rax
  push %rax
  lea -32(%rbp), %rax
  pop %rdi
  add %rdi, %rax
  push %rax
  mov $2, %rax
  pop %rdi
  mov %rax, (%rdi)
  lea -32(%rbp), %rax
  push %rax
  pop %rdi
  mov $0, %rax
  call fun2
  jmp .L.return.main
.L.return.main:
  mov %rbp, %rsp
  pop %rbp
  ret
