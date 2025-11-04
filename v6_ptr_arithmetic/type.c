#include "chibicc.h"

Type *ty_int = &(Type){TY_INT};

// returns 1 if ty's kind is integer, i.e. "TY_INT"
bool is_integer(Type *ty) {
  return ty->kind == TY_INT;
}

/* allocates a Type struct and sets kind to TY_PTR, since it is
 * a pointer, and then sets the base type (what does TY_PTR point to)
 * to the Type specified by "base"
 */
Type *pointer_to(Type *base) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_PTR;
  ty->base = base;
  return ty;
}

/* add_type() does a depth-first traversal of the parse tree pointed by node,
 * and assigns types to each node, depending on the type of that node
 * and other sibling/children nodes, if required
 *
 * the function is a recursive one, and its base case is when either
 * node is NULL, or the type for the node has already been assigned
 */
void add_type(Node *node) {
  if (!node || node->ty)
    return;

  // call add_type() for all the children of the current node
  // before deciding this node's type
  add_type(node->lhs);
  add_type(node->rhs);
  add_type(node->cond);
  add_type(node->then);
  add_type(node->els);
  add_type(node->init);
  add_type(node->inc);

  // traverse the singly linked list that links the expr_stmt style nodes
  for (Node *n = node->body; n; n = n->next)
    add_type(n);

  /* now assign a type to this node
   *
   * 1. 
   * for arithmetic operations, we assign the type to that which is given
   * by the lhs of the node
   *
   * this is also true for assignment nodes :)
   *
   * 2. 
   * for comparison operators, the type should be int, since the answer 
   * of a comparison will always be an integer
   * this also applies for numbers (for now)
   *
   * 3.
   * variables
   * these are by default ints right now, but this will change later
   *
   * 4. 
   * address
   * pointer_to() is called, which creates a new Type structure, whose
   * base will point to the Type struct of this node's lhs
   *
   * 5. 
   * dereference
   * this is an integer, unless we have a pointer to a pointer :) (I guess)
   * in the latter case, we make node->ty point to the base of the lhs' Type
   */
  switch (node->kind) {
  case ND_ADD:
  case ND_SUB:
  case ND_MUL:
  case ND_DIV:
  case ND_NEG:
  case ND_ASSIGN:
    node->ty = node->lhs->ty;
    return;
  case ND_EQ:
  case ND_NE:
  case ND_LT:
  case ND_LE:
  case ND_VAR:
  case ND_NUM:
    node->ty = ty_int;
    return;
  case ND_ADDR:
    node->ty = pointer_to(node->lhs->ty);
    return;
  case ND_DEREF:
    if (node->lhs->ty->kind == TY_PTR)
      node->ty = node->lhs->ty->base;
    else
      node->ty = ty_int;
    return;
  }
}
