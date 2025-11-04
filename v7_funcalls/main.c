#include "chibicc.h"

/* In this version, there are two things that have been added:
 *
 * 1. function definitions and calls upto 6 parameters
 * 2. variables now need to be declared, and are all ints, or pointers
 *
 *
 * according to me, the following things will need to be changed in the code:
 *
 * tokenizer will remain the same, since the task of implementing functions
 * does not require any additional code there
 *
 * the parser will change, since the grammar will now need to consider function
 * calls and definitions
 *
 * fuction calls might look like:
 * these might be given by id (arglist)
 * where arglist is an optional (since zero-arity functions might exist) list
 * of arguments, which are expressions (since expressions are evaluted while
 * parameter passing in C)
 *
 * these will have the highest precedence, and will be in the primary's 
 * production body
 *
 * for function definitions, we might need the same but it should be
 * followed by a block or semicolon (semicolon for declaration of fuction)
 *
 * some checking will need to be done for verifying that the function that
 * has beed called is defined
 *
 * we will also need calling convention code and parameter passing code, which
 * will basically allocate and deallocate the stack frame respectively
 *
 * each function will also have its own local variables
 *
 * for variable declarations compulsory, we will also need a decl-stmt
 * sort of nonterminal in the grammar, which will generate things like
 *
 * "int *a, b;", "int **a, *b, *ptr, q;" etc
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
