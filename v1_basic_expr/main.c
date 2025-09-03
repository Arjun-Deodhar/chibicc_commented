#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// Tokenizer
//

/* Functionality: solve expressions involving +, -, *, / and (, ) which is passed
 * as argv[1]
 *
 * there may be whitespaces which are skipped while tokenising
 */

/* Flow of code
 *
 * Tokenizer generates a singly linked list of Token nodes
 *
 * Parser creates an AST (parse tree from these nodes)
 *
 * Code generator emits assembly instructions to solve the expression
 */

/* Tokenizer creates a singly linked list of Token nodes, which contain
 * kind		the type of token
 * next		pointer to the next Token in the linked list
 * val		its value (if the token is a number)
 * loc		character pointer to the location in the input string
 * len		length of the token 
 */

typedef enum {
  TK_PUNCT, // Punctuators
  TK_NUM,   // Numeric literals
  TK_EOF,   // End-of-file markers
} TokenKind;

// Token type
typedef struct Token Token;
struct Token {
  TokenKind kind; // Token kind
  Token *next;    // Next token
  int val;        // If kind is TK_NUM, its value
  char *loc;      // Token location
  int len;        // Token length
};

// Input string
static char *current_input;

/* just prints the error according to fmt (format)
 *
 *
 * va_start(ap, fmt) creates a variable argument list (mostly) starting
 * at the argument after parameter corresponding to fmt
 *
 * then, the va_list can be passed on to vprintf()
 *
 * vprintf() which prints the string to stderr according
 * to the specified format
 *
 * also, a newline is printed to stderr after printing the error
 * message
 */
// Reports an error and exit.
static void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// Reports an error location and exit.
static void verror_at(char *loc, char *fmt, va_list ap) {
  int pos = loc - current_input;
  fprintf(stderr, "%s\n", current_input);
  fprintf(stderr, "%*s", pos, ""); // print pos spaces.
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

static void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(loc, fmt, ap);
}

static void error_tok(Token *tok, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(tok->loc, fmt, ap);
}

/* this just checks whether op equals tok
 *
 * another check is made to ensure that op[tok->len] is '\0'
 * this is done to ensure that something like the following is not 
 * matched
 *
 * op 		--> "+a"
 * tok->loc	--> "+"
 */

// Consumes the current token if it matches `s`.
static bool equal(Token *tok, char *op) {
  return memcmp(tok->loc, op, tok->len) == 0 && op[tok->len] == '\0';
}

/* skip() just calls equal() and then returns the next token
 */
// Ensure that the current token is `s`.
static Token *skip(Token *tok, char *s) {
  if (!equal(tok, s))
    error_tok(tok, "expected '%s'", s);
  return tok->next;
}

// Ensure that the current token is TK_NUM.
static int get_number(Token *tok) {
  if (tok->kind != TK_NUM)
    error_tok(tok, "expected a number");
  return tok->val;
}

// Create a new token.
static Token *new_token(TokenKind kind, char *start, char *end) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->loc = start;
  tok->len = end - start;
  return tok;
}

/* this is the main tokenizer function which creates the singly linked list
 * of tokens
 *
 * it scans the input does the following
 * (p points to current character being scanned in the input string)
 *
 * while (end of the string is not reached) {
 *		if(p is a whitespace)
 *			skip
 *
 *		else if (p is a digit)
 *			call strtoul(NUM, p, p)
 *			this converts the string after p into an unsigned int
 *			then, p is set to the first invalid character in the unsigned
 *			int
 *
 *			it then returns the unsigned int which can be set to tok->val
 *			create a new token, set tok->val and next pointer as well
 *
 *		else if (p is a punctuator (+, -, ...))
 *			create a punctuator Token node and add it to the singly linked
 *			list
 * }
 * finally, create a new Token node with type EOF, which marks the end of the 
 * singly linked list
 *
 * also, there is a Token head which will point to the first token node created
 * the value returned by tokenize() is head.next
 */

// Tokenize `current_input` and returns new tokens.
static Token *tokenize(void) {
  char *p = current_input;
  Token head = {};
  Token *cur = &head;

  while (*p) {
    // Skip whitespace characters.
    if (isspace(*p)) {
      p++;
      continue;
    }

    // Numeric literal
    if (isdigit(*p)) {
      cur = cur->next = new_token(TK_NUM, p, p);
      char *q = p;
      cur->val = strtoul(p, &p, 10);
      cur->len = p - q;
      continue;
    }

    // Punctuators
    if (ispunct(*p)) {
      cur = cur->next = new_token(TK_PUNCT, p, p + 1);
      p++;
      continue;
    }

    error_at(p, "invalid token");
  }

  cur = cur->next = new_token(TK_EOF, p, p);
  return head.next;
}

//
// Parser
//

/* Grammar of the parser (in an informal sense) is:
 *
 *		expr = mul ("+" mul | "-" mul)*
 *		mul = primary ("*" primary | "/" primary)*
 *		primary = "(" expr ")" | num
 *
 * expr is the start symbol
 *
 * each function for the nonterminals follows a similar pattern
 *
 * expr(Token **rest, Token *tok)
 * 		here, rest is a pointer to a Token *, which is set by expr 
 * 		indicating the remaining linked list of Tokens to be traversed
 *
 * 		tok is the current token from which the parser must generate
 * 		the AST
 *
 * 		now, since expr = mul ("+" mul | "-" mul)*
 *
 * 		we need to first call mul(), which will generate an AST node
 * 		then, in an infinite loop 
 * 		(because the remaining pattern can occur zero or more times)
 *
 * 			if equal("+", tok)
 * 				this means that we need to create an addition AST node
 * 				the left child will be the node returned by previously called
 * 				mul()
 *
 * 				the right child will be the node returned by mul() from the tokens
 * 				after tok
 *
 * 			similar for "-", but a subtraction node is created
 *
 * 			if neither are true, this means that we must return
 * 		
 * 		each function in the parser returns the root of the AST created
 *
 *
 * for invoking the parser, the function that implements the start symbol
 * must be called (which is called in main() in this program)
 *
 * then, for error checking outside the parser, the caller must confirm that
 * the rest pointer set by parser points to the EOF Token (last token in the singly
 * linked list, indicating the end)
 *
 * now, the caller has the parse tree (here, AST) and assembly code can be easily
 * generated from it :)
 */
typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NUM, // Integer
} NodeKind;

// AST node type
typedef struct Node Node;
struct Node {
  NodeKind kind; // Node kind
  Node *lhs;     // Left-hand side
  Node *rhs;     // Right-hand side
  int val;       // Used if kind == ND_NUM
};

// these new_* style functions just create AST nodes by calloc()


// create a new node and set node->kind to `kind`
static Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

// create a binary operand node, whose left and right children are `lhs`
// and `rhs`
static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

// create a number node and then set its value to `val`
static Node *new_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

static Node *expr(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);

// expr = mul ("+" mul | "-" mul)*
static Node *expr(Token **rest, Token *tok) {
  Node *node = mul(&tok, tok);

  for (;;) {
    if (equal(tok, "+")) {
      node = new_binary(ND_ADD, node, mul(&tok, tok->next));
      continue;
    }

    if (equal(tok, "-")) {
      node = new_binary(ND_SUB, node, mul(&tok, tok->next));
      continue;
    }

    *rest = tok;
    return node;
  }
}

// mul = primary ("*" primary | "/" primary)*
static Node *mul(Token **rest, Token *tok) {
  Node *node = primary(&tok, tok);

  for (;;) {
    if (equal(tok, "*")) {
      node = new_binary(ND_MUL, node, primary(&tok, tok->next));
      continue;
    }

    if (equal(tok, "/")) {
      node = new_binary(ND_DIV, node, primary(&tok, tok->next));
      continue;
    }

    *rest = tok;
    return node;
  }
}

/* primary() does not contain an inifinte loop because the grammar does
 * not require that feature :)
 */
// primary = "(" expr ")" | num
static Node *primary(Token **rest, Token *tok) {
  if (equal(tok, "(")) {
    Node *node = expr(&tok, tok->next);
    *rest = skip(tok, ")");
    return node;
  }

  if (tok->kind == TK_NUM) {
    Node *node = new_num(tok->val);
    *rest = tok->next;
    return node;
  }

  error_tok(tok, "expected an expression");
}

//
// Code generator
//

/* this is a VERY interesting and short, crisp code that emits assembly code that
 * will "solve" the AST returned by parser
 *
 * basically, the AST returned by the parser is a postfix expression tree, since 
 * the postorder traversal of that tree will return a postfix expression, which
 * can be evaluated to obtain the result
 *
 * the code generator must write assembly code that will solve the expression
 * if the assembly code is run
 *
 * generally for evalutating such trees, we do:
 * 		1. take the postorder traversal, so that you obtain the postfix expression
 * 		2. initialise a stack
 * 		3. scan the postfix expression from left to right:
 * 			if you find a number
 * 				push it on the stack	
 * 			if you find an operator
 * 				pop the two operands on the stack, 
 * 				apply the operation on them
 * 				push the result back on the stack
 * 		
 * 		4. now, after the entire expression has been scanned, there must be only 
 * 		   one number on the stack (assuming that the postfix expression is correct)
 * 		   this number is the result of the expression
 *
 * Here, we need to emit the code that will "simulate" this solving
 * We must think slightly differently!!
 *
 *
 * %rax stores the final result of the expression, if the assembly code is run
 * we also have a stack!! (which is pointed to by esp :) )
 */
static int depth;

// emit a push %rax instruction
static void push(void) {
  printf("  push %%rax\n");
  depth++;
}

// emit a pop `arg` instruction, where `arg` is some register
static void pop(char *arg) {
  printf("  pop %s\n", arg);
  depth--;
}


/* consider the parse tree generated from the expression 25 + 3 * (92 - 7)
 * 		
 * 			+
 * 	25				*
 * 				3		-
 * 					92		7
 *
 * here, taking the post order traversal, we get
 * 25 3 92 7 - * + 
 *
 * if we push everything on the stack, it will look like
 *
 * 7
 * 92
 * 3
 * 25
 *
 * and then, we find a -, so pop 7 and 92, subtract them and push the result
 * 85 back on the stack
 *
 *
 * the contents of the stack are manipulated as follows
 *
 * 7
 * 92		85		
 * 3	-	3	*	255	+	
 * 25		25		25		275
 *
 * hence, we must simulate this stack getting popped from top to bottom
 *
 * for the example (7 + 2) * (9 - 3)
 *
 * 				*
 * 		+				-
 * 	7		2		9		3
 *
 * 	we will get 7 2 + 9 3 - *
 *
 * 	while solving this, gen_expr() will emit instructions for 9 - 3 first 
 * 	and then 7 - 2, because of the rhs() call being done first and then the lhs()
 *
 * gen_expr() is a recursive function:
 * 		if you find a number, just mov it to %rax, and return
 * 		(base case, since leaves of the parse tree will always be numbers)
 *
 * 		gen_expr() for the right subtree
 * 		push() the result of that on the stack
 * 		gen_expr() for the left subtree
 *		now, we need to apply the operator in the current node on
 *			1. top of the stack 
 *			2. %rax
 *
 *		hence, we pop the stack and emit the arithmetic operation instruction
 *		according to the symbol stored in the node
 */
static void gen_expr(Node *node) {
  if (node->kind == ND_NUM) {
    printf("  mov $%d, %%rax\n", node->val);
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
  }

  // throw an error if none of the nodes are matched
  error("invalid expression");
}

int main(int argc, char **argv) {
  // this error has nothing to do with compiling, it is just for i/o
  if (argc != 2)
    error("%s: invalid number of arguments", argv[0]);

  // Tokenize and parse.
  current_input = argv[1];
  Token *tok = tokenize();
  Node *node = expr(&tok, tok);

  // ensure that the parsing happened properly
  if (tok->kind != TK_EOF)
    error_tok(tok, "extra token");

  printf("  .globl main\n");
  printf("main:\n");

  // Traverse the AST to emit assembly.
  gen_expr(node);
  printf("  ret\n");

  // assert that the depth is zero, meaning that code for the
  // tree was generated successfully
  assert(depth == 0);
  return 0;
}
