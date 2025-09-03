#include "chibicc.h"


/* In this version, there are two things that have been added:
 *
 * 1. while loop 
 * 2. for loop
 *
 * while, for need to be added to the list of keywords
 * parse tree nodes will need to be created for them, and new productions
 * need to be added to the grammar for handling while and for loops
 *
 * an excellent thing is that a single node is used to represent both for and while
 * loops, indicating that they are interconvertible :))
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
