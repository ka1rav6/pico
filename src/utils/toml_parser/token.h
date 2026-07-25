#ifndef PICO_TOML_TOKEN_H
#define PICO_TOML_TOKEN_H

#include <stddef.h>

typedef enum {
  DECIMAL_LITERAL,
  FLOAT_LITERAL,
  STRING_LITERAL,
  BOOL_LITERAL,
  EQUAL_SIGN,
  LSQUARE,
  RSQUARE,
  IDENT,
  COMMA,
  DOT,
  EOF_TOKEN,
} TokenType;

typedef struct Token {
  TokenType type;
  const char *lexeme;
  size_t lexeme_len;
  size_t line;
  size_t col;
} Token;

#endif // PICO_TOML_TOKEN_H
