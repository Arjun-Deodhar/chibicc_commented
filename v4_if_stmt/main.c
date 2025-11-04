#include "chibicc.h"

/* In this version, there are two things that have been added:
 *
 * 1. return statement is supported
 * 2. program can now contain blocks {...}
 * 3. if statement is now supported
 *
 * one change this causes is that now the tokenizer must be
 * able to distinguish between keywords and identifiers so that the
 * parser can proplerly parse the grammar
 *
 * labels will need to be generated for the jumps while converting if
 * statements to assembly
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
