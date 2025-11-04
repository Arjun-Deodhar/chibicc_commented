#include "chibicc.h"

/* in this version, the code is split into multiple files, for 
 * segregating the tokenizer, parser and code generator
 *
 * chibicc.h contains the struct declarations used by all the components
 * of the compiler
 *
 * flow of code is still same
 *
 * improvements from previous version are that it contains some more operators
 * (essentially more levels of precedence) and also supports
 *
 * also, unary minus and plus are supported
 *
 * error printing is more "to the point"
 */
int main(int argc, char **argv) {
  if (argc != 2)
    error("%s: invalid number of arguments", argv[0]);

  Token *tok = tokenize(argv[1]);
  Node *node = parse(tok);
  codegen(node);
  return 0;
}
