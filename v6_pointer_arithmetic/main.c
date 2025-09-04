#include "chibicc.h"

/* In this version, there are three things that have been added:
 *
 * 1. unary & and * operators, used for address and dereference
 * 2. pointer arithmetic :)
 * 3. nodes in the parse tree now have specified types (which are currently just
 *    "int" and "ptr")
 *
 * this commit is very interesting since it implements pointers!!
 * pointer arithmetic means converting addition of numbers to a pointer
 * into the actual meaning
 */
int main(int argc, char **argv) {
  if (argc != 2)
    error("%s: invalid number of arguments", argv[0]);

  Token *tok = tokenize(argv[1]);
  Function *prog = parse(tok);

  // Traverse the AST to emit assembly.
  codegen(prog);

  return 0;
}
