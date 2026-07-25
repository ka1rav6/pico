#ifndef PICO_TOML_LEXER_H
#define PICO_TOML_LEXER_H

#include "token.h"
#include <stdbool.h>
#include <stddef.h>

#define LEXER_MAX_TOKENS 4096

typedef struct {
  const char *src;
  size_t src_len;
  size_t pos;
  size_t line;
  size_t col;
  Token tokens[LEXER_MAX_TOKENS];
  size_t token_count;
  bool has_error;
  char error_msg[256];
  size_t error_line;
  size_t error_col;
} Lexer;

void lexer_init(Lexer *l, const char *src, size_t src_len);
bool lexer_tokenize(Lexer *l);

#endif /* PICO_TOML_LEXER_H */
