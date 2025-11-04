#include "chibicc.h"

static int depth;

/* argument registers, maybe for the calling convention, parameter
 * passing?
 *
 * since there are only 6 registers here, it is said in the commit that
 * "only 6 parameters can be passed (atmost) to functions"
 *
 * Here, if more than 6 parameters are passed, instructions will be generated
 * for them to get pushed on the stack but they will never be stored in argreg
 *
 * the instructions emitted will look like: "mov (null), <offset>(%rbp)"
 *
 * the parameters are initially pushed on the stack and then one by one
 * popped into argregs[i]
 *
 * In assembler, the
 * convention is that the first argument goes into the %rdi register and the 
 * second argument goes into the %rsi register and so on....
 *
 * then, when the function is called, the initial code of the function copies
 * the values from argregs[i] onto the stack, using the offsets computed by
 * assign_lvar_offsets()
 *
 * this is because the parameters are also included in the local variables singly
 * linked list towards the end of the list, and they are assumed to hold position
 * on the stack
 *
 * hence, 
 * for the following function definition:
 * "int fun(int a1, int a2, int a3) { ... }"
 * assume that it is called like "fun(1, 2, 3)"
 *
 * before the function is called, 1, 2, 3 are pushed onto the stack
 * (3 is at the top)
 * now, the stack is popped to load the registers in argregs[] with the values
 * 1, 2 and 3
 *
 * here, the register - value pairs will be
 * %rdi	1
 * %rsi 2
 * %rdx 3
 *
 * now, the call instruction is emitted
 *
 * the first code in the function call (after manipulating rsp and rbp)
 * moves the values in argregs[] onto the stack frame by using the offset
 * for %rbp
 *
 * hence, 
 * %rdi --> -8(%rbp)
 * %rsi --> -16(%rbp)
 * %rdx --> -24(%rbp)
 *
 * now whenever a1 is used in fun(), it will be available as a local variable
 * -8(%rbp)
 * :)
 */
static char *argreg[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
static Function *current_fn;

static void gen_expr(Node *node);

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
  switch (node->kind) {
  case ND_VAR:
    printf("  lea %d(%%rbp), %%rax\n", node->var->offset);
    return;
  case ND_DEREF:
    gen_expr(node->lhs);
    return;
  }

  error_tok(node->tok, "not an lvalue");
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
  case ND_DEREF:
    gen_expr(node->lhs);
    printf("  mov (%%rax), %%rax\n");
    return;
  case ND_ADDR:
    gen_addr(node->lhs);
    return;
  case ND_ASSIGN:
    gen_addr(node->lhs);
    push();
    gen_expr(node->rhs);
    pop("%rdi");
    printf("  mov %%rax, (%%rdi)\n");
    return;
  /* for a function call
   *
   * traverse the arguments list, and generate code for each argument 
   * expression 
   * after that, push() emits the instruction to push the value of %rax (the
   * result of the experssion) on the stack
   * 
   * after that, emit instructions to pop the stack to store the values in 
   * the resgiters specified in argreg[]
   *
   * read about this in the topmost comment, an example is also given
   *
   * finally, mov $0 to %rax and emit a call instruction, for calling
   * the function
   */
  case ND_FUNCALL: {
    int nargs = 0;
    for (Node *arg = node->args; arg; arg = arg->next) {
      gen_expr(arg);
      push();
      nargs++;
    }

    for (int i = nargs - 1; i >= 0; i--)
      pop(argreg[i]);

    printf("  mov $0, %%rax\n");
    printf("  call %s\n", node->funcname);
    return;
  }
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

  error_tok(node->tok, "invalid expression");
}

static void gen_stmt(Node *node) {
  switch (node->kind) {
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
  case ND_FOR: {
    int c = count();
    if (node->init)
      gen_stmt(node->init);
    printf(".L.begin.%d:\n", c);
    if (node->cond) {
      gen_expr(node->cond);
      printf("  cmp $0, %%rax\n");
      printf("  je  .L.end.%d\n", c);
    }
    gen_stmt(node->then);
    if (node->inc)
      gen_expr(node->inc);
    printf("  jmp .L.begin.%d\n", c);
    printf(".L.end.%d:\n", c);
    return;
  }
  case ND_BLOCK:
    for (Node *n = node->body; n; n = n->next)
      gen_stmt(n);
    return; 
  /* there can be many return statements, and hence labels are
   * generated for the return of each function
   */
  case ND_RETURN:
    gen_expr(node->lhs);
    printf("  jmp .L.return.%s\n", current_fn->name);
    return;
  case ND_EXPR_STMT:
    gen_expr(node->lhs);
    return;
  }

  error_tok(node->tok, "invalid statement");
}

// Assign offsets to local variables.
static void assign_lvar_offsets(Function *prog) { 
  /* traverse the singly linked list of function definitions in
   * the program, and for each Function< assign offsets to the
   * local variables in the function
   */
  for (Function *fn = prog; fn; fn = fn->next) {
    int offset = 0;
    for (Obj *var = fn->locals; var; var = var->next) {
      offset += 8;
      var->offset = -offset;
    }
    fn->stack_size = align_to(offset, 16);
  }
}

void codegen(Function *prog) {
  assign_lvar_offsets(prog);

  /* for each Function defined in the program
   *
   * 	add a .globl directive to mark the function name
   * 	as global (for example, .globl main)
   *
   * 	then, add a label for the function name, indicating
   * 	the function starting (main:)
   *
   * 	current_fn is a pointer that points to the
   * 	current function whose code is getting generated
   *
   * 	the Prologue for each function (allocating the stack frame)
   * 	is due to the calling convention :)
   *
   * 	push %rbp			saves the current value of rbp on stack which
   * 						will be popped when the function returns
   *
   * 	mov %rsp, %rbp		sets rsp to point to the current value of rbp
   * 						later on it will be used to restore the stack
   *
   * 	sub $<size>, %rsp	this subtracts the stack_size from rsp, which 
   * 						basically allocates the stack frame for the local
   * 						variables of the function
   * 						this will be "deallocated" when the function 
   * 						returns
   *
   * 	after that, we save all the arguments passed by the resgiters
   * 	in argreg[] to the stack
   *
   * 	they are passed in the registers when the function is called
   * 	
   * 	now, generate code for the function's body
   *
   * 	finally, in the Epilogue part, 
   * 	add a label .L.return.<function_name> (.L.return.main)
   *	deallocate the stack frame by restoring %rsp and %rbp
   *
   *	return instruction is emitted, (return value is put in %rax)
   */
  for (Function *fn = prog; fn; fn = fn->next) {
    printf("  .globl %s\n", fn->name);
    printf("%s:\n", fn->name);
    current_fn = fn;

    // Prologue
    printf("  push %%rbp\n");
    printf("  mov %%rsp, %%rbp\n");
    printf("  sub $%d, %%rsp\n", fn->stack_size);

    // Save passed-by-register arguments to the stack
    int i = 0;
    for (Obj *var = fn->params; var; var = var->next)
      printf("  mov %s, %d(%%rbp)\n", argreg[i++], var->offset);

    // Emit code
    gen_stmt(fn->body);
    assert(depth == 0);

    // Epilogue
    printf(".L.return.%s:\n", fn->name);
    printf("  mov %%rbp, %%rsp\n");
    printf("  pop %%rbp\n");
    printf("  ret\n");
  }
}
