#ifndef PICO_TOML_TOKEN_H
#define PICO_TOML_TOKEN_H

#include <stddef.h>

typedef enum {
  // Literals
  TOK_INTEGER,
  TOK_FLOAT,
  TOK_STRING,
  TOK_BOOL,
  TOK_DATETIME,

  // Identifiers & keys
  TOK_IDENT,

  // Delimiters
  TOK_EQUAL,
  TOK_DOT,
  TOK_COMMA,
  TOK_COLON,

  // Brackets
  TOK_LBRACKET,  // [
  TOK_RBRACKET,  // ]
  TOK_LLBRACKET, // [[
  TOK_RRBRACKET, // ]
  TOK_LBRACE,    // {
  TOK_RBRACE,    // }

  // Special
  TOK_NEWLINE,
  TOK_EOF,
} TokenType;

static const char *token_type_name(TokenType t) {
  switch (t) {
  case TOK_INTEGER:
    return "INTEGER";
  case TOK_FLOAT:
    return "FLOAT";
  case TOK_STRING:
    return "STRING";
  case TOK_BOOL:
    return "BOOL";
  case TOK_DATETIME:
    return "DATETIME";
  case TOK_IDENT:
    return "IDENT";
  case TOK_EQUAL:
    return "EQUAL";
  case TOK_DOT:
    return "DOT";
  case TOK_COMMA:
    return "COMMA";
  case TOK_COLON:
    return "COLON";
  case TOK_LBRACKET:
    return "LBRACKET";
  case TOK_RBRACKET:
    return "RBRACKET";
  case TOK_LLBRACKET:
    return "LLBRACKET";
  case TOK_RRBRACKET:
    return "RRBRACKET";
  case TOK_LBRACE:
    return "LBRACE";
  case TOK_RBRACE:
    return "RBRACE";
  case TOK_NEWLINE:
    return "NEWLINE";
  case TOK_EOF:
    return "EOF";
  }
  return "UNKNOWN";
}

typedef struct Token {
  TokenType type;
  const char *start;
  size_t length;
  size_t line;
  size_t col;
} Token;

#endif // PICO_TOML_TOKEN_H
