#include "chibicc.h"

/* 
 * additions in the last few commits:
 *
 * 	sizeof operator 
 * 	global variales
 * 	char, string literals and escape sequeces 
 * 	code to be compiled is now read from a file
 */

int main(int argc, char **argv) {
  if (argc != 2)
    error("%s: invalid number of arguments", argv[0]);

  // Tokenize and parse.
  Token *tok = tokenize_file(argv[1]);
  Obj *prog = parse(tok);

  // Traverse the AST to emit assembly.
  codegen(prog);

  return 0;
}
