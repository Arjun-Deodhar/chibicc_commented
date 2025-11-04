#include "chibicc.h"

/* code generator needs to handle 
 *
 * 1. return statements
 * return node in the parse tree will have an expression node as a child
 * basically the code generator needs to generate code for that expression
 * and then just jump to a ret instruction after storing the result of that
 * expression in %rax
 *
 * 2. if statement
 * here, the if node will have a cond, which will be an expr
 * code for expr will need to be generated, after which the value is stored
 * in some register
 *
 * assuming the value is stored in %rax
 * then, a label will need to be generated to skip through all that code
 * for evaluating cond
 *
 * this label must be unique and hence some label generator function will
 * be required
 *
 * if there is an else for the if statement, :) then we need to generate another
 * label to skip over the stmt after the if (expr)
 *
 * 3. null statements
 * these can just be skipped
 */
static int depth;

/* used to generate unique labels
 */
static int count(void) {
  static int i = 1;
  return i++;
}

static void push(void) {
  printf("  push %%rax\n");
  depth++;
}

static void pop(char *arg) {
  printf("  pop %s\n", arg);
  depth--;
}

// Round up `n` to the nearest multiple of `align`. For instance,
// align_to(5, 8) returns 8 and align_to(11, 8) returns 16.
static int align_to(int n, int align) {
  return (n + align - 1) / align * align;
}

// Compute the absolute address of a given node.
// It's an error if a given node does not reside in memory.
static void gen_addr(Node *node) {
  if (node->kind == ND_VAR) {
    printf("  lea %d(%%rbp), %%rax\n", node->var->offset);
    return;
  }

  error("not an lvalue");
}

// Generate code for a given node.
static void gen_expr(Node *node) {
  switch (node->kind) {
  case ND_NUM:
    printf("  mov $%d, %%rax\n", node->val);
    return;
  case ND_NEG:
    gen_expr(node->lhs);
    printf("  neg %%rax\n");
    return;
  case ND_VAR:
    gen_addr(node);
    printf("  mov (%%rax), %%rax\n");
    return;
  case ND_ASSIGN:
    gen_addr(node->lhs);
    push();
    gen_expr(node->rhs);
    pop("%rdi");
    printf("  mov %%rax, (%%rdi)\n");
    return;
  }

  gen_expr(node->rhs);
  push();
  gen_expr(node->lhs);
  pop("%rdi");

  switch (node->kind) {
  case ND_ADD:
    printf("  add %%rdi, %%rax\n");
    return;
  case ND_SUB:
    printf("  sub %%rdi, %%rax\n");
    return;
  case ND_MUL:
    printf("  imul %%rdi, %%rax\n");
    return;
  case ND_DIV:
    printf("  cqo\n");
    printf("  idiv %%rdi\n");
    return;
  case ND_EQ:
  case ND_NE:
  case ND_LT:
  case ND_LE:
    printf("  cmp %%rdi, %%rax\n");

    if (node->kind == ND_EQ)
      printf("  sete %%al\n");
    else if (node->kind == ND_NE)
      printf("  setne %%al\n");
    else if (node->kind == ND_LT)
      printf("  setl %%al\n");
    else if (node->kind == ND_LE)
      printf("  setle %%al\n");

    printf("  movzb %%al, %%rax\n");
    return;
  }

  error("invalid expression");
}

static void gen_stmt(Node *node) {
  switch (node->kind) {
  /* for an if statement node
   * generate assembly code for expr statement, such that
   * result is in %rax
   *
   * then, compare the %rax value with 0
   * if the condition expression evaluates to 0, i.e. the
   * condition is false, then jump to the else part which is given
   * by an .L.else.<id> where <id> is some unique number given to 
   * that else statement
   *
   * then, generate assembly code for the then statement (this is a stmt,
   * and hence can be a block)
   *
   * finally, generate an instruction to jump to the .L.end.<id> label 
   * which will skip over the else part
   *
   * insert the .L.else.<id> label here
   * now, generate code for the else part, similar to the then part
   * this is done only if the else parse tree exists
   *
   * insert the .L.end.<id> label here
   */
  case ND_IF: {
    int c = count();
    gen_expr(node->cond);
    printf("  cmp $0, %%rax\n");
    printf("  je  .L.else.%d\n", c);
    gen_stmt(node->then);
    printf("  jmp .L.end.%d\n", c);
    printf(".L.else.%d:\n", c);
    if (node->els)
      gen_stmt(node->els);
    printf(".L.end.%d:\n", c);
    return;
  }
  /* for a block node, call gen_stmt() for each of the statement
   * parse trees which are connected in the singly linked list
   * fashion
   */
  case ND_BLOCK:
    for (Node *n = node->body; n; n = n->next)
      gen_stmt(n);
    return;
  /* for a return node, just geenrate an instruction that jumps to 
   * ret instruction, label .L.return which will typically be at
   * the end of the assembly program
   */
  case ND_RETURN:
    gen_expr(node->lhs);
    printf("  jmp .L.return\n");
    return;
  case ND_EXPR_STMT:
    gen_expr(node->lhs);
    return;
  }

  error("invalid statement");
}

// Assign offsets to local variables.
static void assign_lvar_offsets(Function *prog) {
  int offset = 0;
  for (Obj *var = prog->locals; var; var = var->next) {
    offset += 8;
    var->offset = -offset;
  }
  prog->stack_size = align_to(offset, 16);
}

void codegen(Function *prog) {
  assign_lvar_offsets(prog);

  printf("  .globl main\n");
  printf("main:\n");

  // Prologue
  printf("  push %%rbp\n");
  printf("  mov %%rsp, %%rbp\n");
  printf("  sub $%d, %%rsp\n", prog->stack_size);

  gen_stmt(prog->body);
  assert(depth == 0);

  /* insert label .L.return to which all the return statements in the C
   * program will jump to
   */
  printf(".L.return:\n");

  /* calling convetion to deallocate local varialbes and restore the value
   * of %rsp, thus deallocating the stack frame
   *
   * return value is stored in %rax
   */
  printf("  mov %%rbp, %%rsp\n");
  printf("  pop %%rbp\n");
  printf("  ret\n");
}
