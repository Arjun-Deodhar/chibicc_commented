#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node Node;

//
// tokenize.c
//

// Token
typedef enum {
  TK_IDENT, // Identifiers
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

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
bool equal(Token *tok, char *op);
Token *skip(Token *tok, char *op);
Token *tokenize(char *input);

//
// parse.c
//

/* this structure is used for local variables, which are stored in a linked list
 * name is a pointer to the string which is the name of the identifier
 * offset is the offset from the %rbp stack pointer, which points to the bottom
 * of the stack frame
 *
 * since local variable of a function are accessed relative to the stack frame 
 * allocated to that function (on calling), uses of that identifier will lead
 * to accessing the stack!
 */
// Local variable
typedef struct Obj Obj;
struct Obj {
  Obj *next;
  char *name; // Variable name
  int offset; // Offset from RBP
};

/* function structure points to a parse tree node, which we can assume is the root
 * node for that functions
 *
 * there is also a pointer to the linked list of local variables that are defined
 * in that function, along with the stack_size required for that function (maybe the
 * stack pointer %rsp will need to be decremented by those many bytes for allocating
 * space for local variables on the stack)
 *
 * right now there is no notion of a function as such, but it is assumed that all the
 * statements in the program are part of one big function
 */
// Function
typedef struct Function Function;
struct Function {
  Node *body;
  Obj *locals;
  int stack_size;
};

// AST node
/* an interesting thing is that they have used only one node type for "<" as well as ">"
 * similarly for "<=" and ">="
 *
 * they just swap the left and right children to make a ">=" act like a "<=" :)
 */
typedef enum {
  ND_ADD,       // +
  ND_SUB,       // -
  ND_MUL,       // *
  ND_DIV,       // /
  ND_NEG,       // unary -
  ND_EQ,        // ==
  ND_NE,        // !=
  ND_LT,        // <
  ND_LE,        // <=
  ND_ASSIGN,    // =
  ND_EXPR_STMT, // Expression statement
  ND_VAR,       // Variable
  ND_NUM,       // Integer
} NodeKind;

// AST node type
/* some additional node types need to be considered, such as variable
 * and assignment
 *
 * the next pointer is used to chain all the expr-stmt nodes which are the
 * roots of parse trees for each statement
 */
struct Node {
  NodeKind kind; // Node kind
  Node *next;    // Next node
  Node *lhs;     // Left-hand side
  Node *rhs;     // Right-hand side
  Obj *var;      // Used if kind == ND_VAR
  int val;       // Used if kind == ND_NUM
};

Function *parse(Token *tok);

//
// codegen.c
//

void codegen(Function *prog);
