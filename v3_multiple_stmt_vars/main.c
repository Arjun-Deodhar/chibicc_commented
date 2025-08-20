#include "chibicc.h"

/* In this version, there are two things that have been added:
 *
 * 1. multiple statements separated by semicolons are now allowed
 * 2. support for multi-letter local variables, like "a = 5;"
 *
 *
 * for adding these features, the parser will need to parse each 
 * stmt separately, and hence the parse tree will is longer be a 
 * "pure" tree (data structure wise)
 *
 * there need to be lots of additions like how to store the info about
 * local variables, how to generate %rbp offsets for those local variables,
 * allocating the stack frame for function calls etc.
 *
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
