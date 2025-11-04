  .globl main
main:
  push %rbp
  mov %rsp, %rbp
  sub $16, %rsp
  lea -8(%rbp), %rax
  push %rax
  mov $0, %rax
  pop %rdi
  mov %rax, (%rdi)
.L.begin.1:
  mov $10, %rax
  push %rax
  lea -8(%rbp), %rax
  mov (%rax), %rax
  pop %rdi
  cmp %rdi, %rax
  setl %al
  movzb %al, %rax
  cmp $0, %rax
  je  .L.end.1
  lea -8(%rbp), %rax
  push %rax
  mov $1, %rax
  push %rax
  lea -8(%rbp), %rax
  mov (%rax), %rax
  pop %rdi
  add %rdi, %rax
  pop %rdi
  mov %rax, (%rdi)
  jmp .L.begin.1
.L.end.1:
  lea -8(%rbp), %rax
  mov (%rax), %rax
  jmp .L.return
.L.return:
  mov %rbp, %rsp
  pop %rbp
  ret
