#include "chibicc.h"

// All local variable instances created during parsing are
// accumulated to this list.
Obj *locals;

static Node *expr(Token **rest, Token *tok);
static Node *expr_stmt(Token **rest, Token *tok);
static Node *assign(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);

// Find a local variable by name.
static Obj *find_var(Token *tok) {
  for (Obj *var = locals; var; var = var->next)
    if (strlen(var->name) == tok->len && !strncmp(tok->loc, var->name, tok->len))
      return var;
  return NULL;
}

static Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_unary(NodeKind kind, Node *expr) {
  Node *node = new_node(kind);
  node->lhs = expr;
  return node;
}

static Node *new_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

static Node *new_var_node(Obj *var) {
  Node *node = new_node(ND_VAR);
  node->var = var;
  return node;
}

static Obj *new_lvar(char *name) {
  Obj *var = calloc(1, sizeof(Obj));
  var->name = name;
  var->next = locals;
  locals = var;
  return var;
}

/* Grammar that is used by the parser is now:
 *
 *
 * 1. stmt is either expression or statement
 * stmt = expr-stmt
 *
 * 2. expr-stmt is an expr followed by a semicolon
 * expr-stmt = expr ";"
 *
 * Q: Why not just do `stmt = expr ";"`
 *
 * 3. expr is assignment
 * expr = assign
 *
 * 4. assignment is equality followed by multiple assignments
 * assign = equality ("=" assign)?
 *
 * 5. now onwards there are nonterminals for each level of precedence until
 *	  the primary
 * equality = relational ("==" relational | "!=" relational)*
 * relational = add ("<" add | "<=" add | ">" add | ">=" add)*
 * add = mul ("+" mul | "-" mul)*
 * mul = unary ("*" unary | "/" unary)*
 * unary = ("+" | "-") unary
 *			 | primary
 *
 * 6. a primary can also be an identifier of a number
 * primary = "(" expr ")" | ident | num
 *
 * 7. program is a sequence of statements
 * program = stmt*
 *
 * although there is no program() function for the nonterminal, program() is
 * basically the parse() function that is called in main() :)
 */

// stmt = expr-stmt
static Node *stmt(Token **rest, Token *tok) {
  return expr_stmt(rest, tok);
}

// expr-stmt = expr ";"
static Node *expr_stmt(Token **rest, Token *tok) {
  Node *node = new_unary(ND_EXPR_STMT, expr(&tok, tok));
  *rest = skip(tok, ";");
  return node;
}

// expr = assign
static Node *expr(Token **rest, Token *tok) {
  return assign(rest, tok);
}

// assign = equality ("=" assign)?
static Node *assign(Token **rest, Token *tok) {
  Node *node = equality(&tok, tok);
  if (equal(tok, "="))
    node = new_binary(ND_ASSIGN, node, assign(&tok, tok->next));
  *rest = tok;
  return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(Token **rest, Token *tok) {
  Node *node = relational(&tok, tok);

  for (;;) {
    if (equal(tok, "==")) {
      node = new_binary(ND_EQ, node, relational(&tok, tok->next));
      continue;
    }

    if (equal(tok, "!=")) {
      node = new_binary(ND_NE, node, relational(&tok, tok->next));
      continue;
    }

    *rest = tok;
    return node;
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Token **rest, Token *tok) {
  Node *node = add(&tok, tok);

  for (;;) {
    if (equal(tok, "<")) {
      node = new_binary(ND_LT, node, add(&tok, tok->next));
      continue;
    }

    if (equal(tok, "<=")) {
      node = new_binary(ND_LE, node, add(&tok, tok->next));
      continue;
    }

    if (equal(tok, ">")) {
      node = new_binary(ND_LT, add(&tok, tok->next), node);
      continue;
    }

    if (equal(tok, ">=")) {
      node = new_binary(ND_LE, add(&tok, tok->next), node);
      continue;
    }

    *rest = tok;
    return node;
  }
}

// add = mul ("+" mul | "-" mul)*
static Node *add(Token **rest, Token *tok) {
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

// mul = unary ("*" unary | "/" unary)*
static Node *mul(Token **rest, Token *tok) {
  Node *node = unary(&tok, tok);

  for (;;) {
    if (equal(tok, "*")) {
      node = new_binary(ND_MUL, node, unary(&tok, tok->next));
      continue;
    }

    if (equal(tok, "/")) {
      node = new_binary(ND_DIV, node, unary(&tok, tok->next));
      continue;
    }

    *rest = tok;
    return node;
  }
}

// unary = ("+" | "-") unary
//       | primary
static Node *unary(Token **rest, Token *tok) {
  if (equal(tok, "+"))
    return unary(rest, tok->next);

  if (equal(tok, "-"))
    return new_unary(ND_NEG, unary(rest, tok->next));

  return primary(rest, tok);
}

// primary = "(" expr ")" | ident | num
static Node *primary(Token **rest, Token *tok) {
  if (equal(tok, "(")) {
    Node *node = expr(&tok, tok->next);
    *rest = skip(tok, ")");
    return node;
  }

  /* if an identifier is found, then the variable name is searched in the local
   * variable list
   *
   * if the variable struct already exists, then a pointer to that is used to
   * create a VAR node, which is then returned by primary()
   *
   * else a new local variable struct is allocated, and is added at the head of
   * the local variable list
   */
  if (tok->kind == TK_IDENT) {
    Obj *var = find_var(tok);
    if (!var)
      var = new_lvar(strndup(tok->loc, tok->len));
    *rest = tok->next;
    return new_var_node(var);
  }

  if (tok->kind == TK_NUM) {
    Node *node = new_num(tok->val);
    *rest = tok->next;
    return node;
  }

  // if neither of these tokens are match, throw the error that an expression was
  // expected
  error_tok(tok, "expected an expression");
}

// program = stmt*
Function *parse(Token *tok) {
  Node head = {};
  Node *cur = &head;

  /* this loop basically does:
   * 	parse the current linked list of tokens pointed to by tok
   * 	(parse them as a stmt())
   *
   * 	then, stmt() will set the tok to point to the next statement's
   * 	tokens
   *
   * until TK_EOF token is encountered
   *
   * i.e. this loop is implementing the program = stmt* production in 
   * the grammar :)
   *
   * the next pointers in the parse tree nodes connect the root nodes of
   * each expr-stmt, forming a singly linked list of all the expr-stmt
   * nodes which is passed on to the code generator
   */
  while (tok->kind != TK_EOF)
    cur = cur->next = stmt(&tok, tok);

  /* although we do not have any notion of defined functions, we are considering
   * the parse tree as part of one big function, with local variables and a body
   */
  Function *prog = calloc(1, sizeof(Function));
  prog->body = head.next;
  prog->locals = locals;
  return prog;
}
