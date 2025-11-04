#include "chibicc.h"

/* since we need to support identifiers, there are new functions 
 * is_ident1() and is_ident2()
 * an identifier is a letter or underscore followed by more letters, underscores
 * or digits
 *
 * is_ident1() just checks for letter and underscore, whereas is_ident2() checks
 * for letter, underscore or digit
 *
 * in the tokenizer()'s loop, an extra case is added for is_ident1(), and if that is
 * true, the remaining identifier is read by calling is_ident2() in a loop until the
 * identifier ends
 *
 * statements are separated by a semicolon, which in this case is just a punctuator
 */
// Input string
static char *current_input;

// Reports an error and exit.
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// Reports an error location and exit.
static void verror_at(char *loc, char *fmt, va_list ap) {
  int pos = loc - current_input;
  // print the input string
  fprintf(stderr, "%s\n", current_input);

  /* "%*s" means align from left by pos, which is a number
   * essentially there will be pos - strlen("") spaces from the left, which
   * is basically pos only
   *
   * as a result, pos spaces are printed at the beginning of the line
   * this is used to align the error caret pointer with the location of the
   * error in the input string, which is set up by the tokenizer
   */
  fprintf(stderr, "%*s", pos, ""); // print pos spaces.
  fprintf(stderr, "^ ");

  // now print the message
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

/* just calls va_start() and passes the parameters to verror_at()
 *
 * same for error_tok(), but the only difference is that error_tok()
 * passes the location as a parameter tok->loc while error_at() can 
 * directly pass it
 */ 
void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(loc, fmt, ap);
}

void error_tok(Token *tok, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(tok->loc, fmt, ap);
}

// Consumes the current token if it matches `op`.
bool equal(Token *tok, char *op) {
  return memcmp(tok->loc, op, tok->len) == 0 && op[tok->len] == '\0';
}

// Ensure that the current token is `op`.
Token *skip(Token *tok, char *op) {
  // throw an error if the token is not equal to the operator specified by op
  // for example, a missing semicolon
  if (!equal(tok, op))
    error_tok(tok, "expected '%s'", op);
  return tok->next;
}

// Create a new token.
static Token *new_token(TokenKind kind, char *start, char *end) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->loc = start;
  tok->len = end - start;
  return tok;
}

static bool startswith(char *p, char *q) {
  return strncmp(p, q, strlen(q)) == 0;
}

// Returns true if c is valid as the first character of an identifier.
static bool is_ident1(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

// Returns true if c is valid as a non-first character of an identifier.
static bool is_ident2(char c) {
  return is_ident1(c) || ('0' <= c && c <= '9');
}

// Read a punctuator token from p and returns its length.
static int read_punct(char *p) {
  if (startswith(p, "==") || startswith(p, "!=") ||
      startswith(p, "<=") || startswith(p, ">="))
    return 2;

  return ispunct(*p) ? 1 : 0;
}

// Tokenize `current_input` and returns new tokens.
Token *tokenize(char *p) {
  current_input = p;
  Token head = {};
  Token *cur = &head;

  while (*p) {
    // Skip whitespace characters.
    if (isspace(*p)) {
      p++;
      continue;
    }

    // Numeric literal
    if (isdigit(*p)) {
      cur = cur->next = new_token(TK_NUM, p, p);
      char *q = p;
      cur->val = strtoul(p, &p, 10);
      cur->len = p - q;
      continue;
    }

    // Identifier
    if (is_ident1(*p)) {
      char *start = p;
      do {
        p++;
      } while (is_ident2(*p));
      cur = cur->next = new_token(TK_IDENT, start, p);
      continue;
    }

    // Punctuators
    int punct_len = read_punct(p);
    if (punct_len) {
      cur = cur->next = new_token(TK_PUNCT, p, p + punct_len);
      p += cur->len;
      continue;
    }

	// Q: Is it even possible to make the code reach here such that this error
	// is thrown?
    error_at(p, "invalid token");
  }

  cur = cur->next = new_token(TK_EOF, p, p);
  return head.next;
}
