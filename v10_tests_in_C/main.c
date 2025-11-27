#include "chibicc.h"

/* added the following funcitonality:
 *
 * 1. comments are handled
 * 2. -o and --help options supported
 * 3. line and block comments (handled by lexer)
 * 4. tests are now written in C instead of shell scripts
 * 5. code to be compiled is read from a file insted of argv[1]
 */

// file name in which the output is to be redirected, if -o specified
static char *opt_o;

// file name from which input is to be taken
static char *input_path;

// prints the usage message and exit() s with the given status code
static void usage(int status) {
  fprintf(stderr, "chibicc [ -o <path> ] <file>\n");
  exit(status);
}

// parses the argument list given to main and sets the proper
// option variables
static void parse_args(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {

		// --help encountered, then just print usage and exit(0)
    if (!strcmp(argv[i], "--help"))
      usage(0);

		// -o, set the opt_o to argv[i]
    if (!strcmp(argv[i], "-o")) {
			// no filename provided to redirect output, exit(1)
      if (!argv[++i])
        usage(1);
      opt_o = argv[i];
      continue;
    }

		// not sure about this right now
    if (!strncmp(argv[i], "-o", 2)) {
      opt_o = argv[i] + 2;
      continue;
    }

		// unkwown argument
    if (argv[i][0] == '-' && argv[i][1] != '\0')
      error("unknown argument: %s", argv[i]);

		// in the end, set the input_path 
    input_path = argv[i];
  }

	// no input_path specified 
  if (!input_path)
    error("no input files");
}

// opens the file to which output needs to be redirected
static FILE *open_file(char *path) {
  if (!path || strcmp(path, "-") == 0)
    return stdout;

  FILE *out = fopen(path, "w");
  if (!out)
    error("cannot open output file: %s: %s", path, strerror(errno));
  return out;
}

int main(int argc, char **argv) {
  parse_args(argc, argv);

  // Tokenize and parse.
  Token *tok = tokenize_file(input_path);
  Obj *prog = parse(tok);

  // Traverse the AST to emit assembly.
  FILE *out = open_file(opt_o);
  codegen(prog, out);
  return 0;
}
