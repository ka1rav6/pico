
#include "lexer.h"
#include <string.h>

void lexer_init(Lexer *l, const char *src, size_t src_len) {
  l->src = src;
  l->src_len = src_len;
  l->pos = 0;
  l->col = 1;
  l->line = 1;
  l->token_count = 0;
  l->has_error = false;
  l->error_msg[0] = '\0';
  l->error_line = 0;
  l->error_col = 0;
}

bool lexer_tokenize(Lexer *l) {
  while (!l->has_error) {
    if (!lexer_next_token(l)) { // TODO: create this
      if (l->has_error)
        return false;
    }
    break;
  }
  return !l->has_error;
}
