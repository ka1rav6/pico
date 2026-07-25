
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

static inline char lexer_peek(const Lexer *l) {
  if (l->pos >= l->src_len)
    return '\0';
  return l->src[l->pos];
}

static inline char lexer_peek_at(const Lexer *l, size_t offset) {
  size_t idx = l->pos + offset;
  if (idx >= l->src_len)
    return '\0';
  return l->src[idx];
}

static inline char lexer_advance(Lexer *l) {
  char c = lexer_peek(l);
  if (c == '\0')
    return c;
  l->pos++;
  if (c == '\n') {
    l->line++;
    l->col = 1;
  } else {
    l->col++;
  }
  return c;
}

static inline bool lexer_match(Lexer *l, char expected) {
  if (lexer_peek(l) != expected)
    return false;
  lexer_advance(l);
  return true;
}

static inline void lexer_set_error(Lexer *l, const char *msg) {
  l->has_error = true;
  size_t i = 0;
  while (msg[i] && i < sizeof(l->error_msg) - 1) {
    l->error_msg[i] = msg[i];
    i++;
  }
  l->error_msg[i] = '\0';
  l->error_line = l->line;
  l->error_col = l->col;
}

static inline bool lexer_add_token(Lexer *l, TokenType type, const char *start,
                                   size_t length, size_t line, size_t col) {
  if (l->token_count >= LEXER_MAX_TOKENS) {
    lexer_set_error(l, "too many tokens");
    return false;
  }
  Token *t = &l->tokens[l->token_count++];
  t->type = type;
  t->start = start;
  t->length = length;
  t->line = line;
  t->col = col;
  return true;
}

static inline bool is_ascii_alpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline bool is_ascii_digit(char c) { return c >= '0' && c <= '9'; }

static inline bool is_ascii_hex_digit(char c) {
  return is_ascii_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static inline bool is_bare_key_char(char c) {
  return is_ascii_alpha(c) || is_ascii_digit(c) || c == '-' || c == '_';
}

static inline bool lexer_skip_whitespace_and_comments(Lexer *l) {
  while (l->pos < l->src_len) {
    char c = lexer_peek(l);
    if (c == ' ' || c == '\t') {
      lexer_advance(l);
    } else if (c == '#') {
      while (l->pos < l->src_len && lexer_peek(l) != '\n')
        lexer_advance(l);
    } else {
      break;
    }
  }
  return !l->has_error;
}

static inline bool lexer_match_keyword(const Lexer *l, const char *kw,
                                       size_t kw_len) {
  if ((size_t)(l->src_len - l->pos) < kw_len)
    return false;
  return memcmp(l->src + l->pos, kw, kw_len) == 0 &&
         !is_bare_key_char(l->src[l->pos + kw_len]);
}
static inline bool lexer_read_string_basic(Lexer *l) {
  size_t sl = l->line, sc = l->col;
  const char *sp = l->src + l->pos;
  lexer_advance(l);
  while (l->pos < l->src_len) {
    char c = lexer_peek(l);
    if (c == '"') {
      const char *end = l->src + l->pos;
      lexer_advance(l);
      return lexer_add_token(l, TOK_STRING, sp, (size_t)(end - sp) + 1, sl, sc);
    } else if (c == '\\') {
      lexer_advance(l);
      if (l->pos < l->src_len) {
        char esc = lexer_advance(l);
        switch (esc) {
        case '"':
        case '\\':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
          break;
        case 'u':
          for (int i = 0; i < 4 && l->pos < l->src_len; i++)
            if (!is_ascii_hex_digit(lexer_advance(l))) {
              lexer_set_error(l, "invalid \\u escape");
              return false;
            }
          break;
        case 'U':
          for (int i = 0; i < 8 && l->pos < l->src_len; i++)
            if (!is_ascii_hex_digit(lexer_advance(l))) {
              lexer_set_error(l, "invalid \\U escape");
              return false;
            }
          break;
        default:
          lexer_set_error(l, "invalid escape character");
          return false;
        }
      }
    } else if (c == '\n') {
      lexer_set_error(l, "unterminated basic string");
      return false;
    } else {
      lexer_advance(l);
    }
  }
  lexer_set_error(l, "unterminated basic string");
  return false;
}

static inline bool lexer_read_string_literal(Lexer *l) {
  size_t sl = l->line, sc = l->col;
  const char *sp = l->src + l->pos;
  lexer_advance(l);
  while (l->pos < l->src_len) {
    char c = lexer_peek(l);
    if (c == '\'') {
      const char *end = l->src + l->pos;
      lexer_advance(l);
      return lexer_add_token(l, TOK_STRING, sp, (size_t)(end - sp) + 1, sl, sc);
    } else if (c == '\n') {
      lexer_set_error(l, "unterminated literal string");
      return false;
    } else {
      lexer_advance(l);
    }
  }
  lexer_set_error(l, "unterminated literal string");
  return false;
}

static inline bool lexer_read_ml_basic_string(Lexer *l) {
  size_t sl = l->line, sc = l->col;
  const char *sp = l->src + l->pos;
  lexer_advance(l);
  lexer_advance(l);
  lexer_advance(l);
  if (lexer_peek(l) == '\n')
    lexer_advance(l);
  while (l->pos < l->src_len) {
    char c = lexer_peek(l);
    if (c == '"') {
      if (lexer_peek_at(l, 1) == '"' && lexer_peek_at(l, 2) == '"') {
        const char *end = l->src + l->pos;
        lexer_advance(l);
        lexer_advance(l);
        lexer_advance(l);
        return lexer_add_token(l, TOK_STRING, sp, (size_t)(end - sp) + 3, sl,
                               sc);
      } else {
        lexer_advance(l);
      }
    } else if (c == '\\') {
      lexer_advance(l);
      if (l->pos < l->src_len) {
        char esc = lexer_advance(l);
        switch (esc) {
        case '"':
        case '\\':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
        case '\n':
          break;
        case 'u':
          for (int i = 0; i < 4 && l->pos < l->src_len; i++)
            if (!is_ascii_hex_digit(lexer_advance(l))) {
              lexer_set_error(l, "invalid escape in multiline");
              return false;
            }
          break;
        case 'U':
          for (int i = 0; i < 8 && l->pos < l->src_len; i++)
            if (!is_ascii_hex_digit(lexer_advance(l))) {
              lexer_set_error(l, "invalid escape in multiline");
              return false;
            }
          break;
        default:
          lexer_set_error(l, "invalid escape in multiline string");
          return false;
        }
      }
    } else {
      lexer_advance(l);
    }
  }
  lexer_set_error(l, "unterminated multiline basic string");
  return false;
}

static inline bool lexer_read_ml_literal_string(Lexer *l) {
  size_t sl = l->line, sc = l->col;
  const char *sp = l->src + l->pos;
  lexer_advance(l);
  lexer_advance(l);
  lexer_advance(l);
  if (lexer_peek(l) == '\n')
    lexer_advance(l);
  while (l->pos < l->src_len) {
    char c = lexer_peek(l);
    if (c == '\'') {
      if (lexer_peek_at(l, 1) == '\'' && lexer_peek_at(l, 2) == '\'') {
        const char *end = l->src + l->pos;
        lexer_advance(l);
        lexer_advance(l);
        lexer_advance(l);
        return lexer_add_token(l, TOK_STRING, sp, (size_t)(end - sp) + 3, sl,
                               sc);
      } else {
        lexer_advance(l);
      }
    } else {
      lexer_advance(l);
    }
  }
  lexer_set_error(l, "unterminated multiline literal string");
  return false;
}
static inline bool lexer_read_number(Lexer *l) {
  size_t sl = l->line, sc = l->col;
  const char *sp = l->src + l->pos;
  bool is_float = false;
  if (l->pos + 2 < l->src_len) {
    const char *p = l->src + l->pos;
    if ((p[0] == 'i' && p[1] == 'n' && p[2] == 'f') ||
        (p[0] == 'n' && p[1] == 'a' && p[2] == 'n')) {
      for (int i = 0; i < 3; i++)
        lexer_advance(l);
      if (lexer_peek(l) == '+' || lexer_peek(l) == '-')
        lexer_advance(l);
      return lexer_add_token(l, TOK_FLOAT, sp, (size_t)((l->src + l->pos) - sp),
                             sl, sc);
    }
  }
  if (lexer_peek(l) == '+' || lexer_peek(l) == '-')
    lexer_advance(l);
  bool is_hex = false, is_octal = false, is_binary = false;
  if (lexer_peek(l) == '0' &&
      (lexer_peek_at(l, 1) == 'x' || lexer_peek_at(l, 1) == 'X')) {
    lexer_advance(l);
    lexer_advance(l);
    is_hex = true;
  } else if (lexer_peek(l) == '0' &&
             (lexer_peek_at(l, 1) == 'o' || lexer_peek_at(l, 1) == 'O')) {
    lexer_advance(l);
    lexer_advance(l);
    is_octal = true;
  } else if (lexer_peek(l) == '0' &&
             (lexer_peek_at(l, 1) == 'b' || lexer_peek_at(l, 1) == 'B')) {
    lexer_advance(l);
    lexer_advance(l);
    is_binary = true;
  }
  if (is_hex) {
    while (l->pos < l->src_len) {
      char c = lexer_peek(l);
      if (is_ascii_hex_digit(c) || c == '_')
        lexer_advance(l);
      else
        break;
    }
  } else if (is_octal) {
    while (l->pos < l->src_len) {
      char c = lexer_peek(l);
      if ((c >= '0' && c <= '7') || c == '_')
        lexer_advance(l);
      else
        break;
    }
  } else if (is_binary) {
    while (l->pos < l->src_len) {
      char c = lexer_peek(l);
      if (c == '0' || c == '1' || c == '_')
        lexer_advance(l);
      else
        break;
    }
  } else {
    while (l->pos < l->src_len) {
      char c = lexer_peek(l);
      if (is_ascii_digit(c) || c == '_')
        lexer_advance(l);
      else
        break;
    }
    if (lexer_peek(l) == '.' && l->pos + 1 < l->src_len &&
        is_ascii_digit(lexer_peek_at(l, 1))) {
      is_float = true;
      lexer_advance(l);
      while (l->pos < l->src_len) {
        char c = lexer_peek(l);
        if (is_ascii_digit(c) || c == '_')
          lexer_advance(l);
        else
          break;
      }
    }
    if (lexer_peek(l) == 'e' || lexer_peek(l) == 'E') {
      is_float = true;
      lexer_advance(l);
      if (lexer_peek(l) == '+' || lexer_peek(l) == '-')
        lexer_advance(l);
      while (l->pos < l->src_len && is_ascii_digit(lexer_peek(l)))
        lexer_advance(l);
    }
  }
  return lexer_add_token(l, is_float ? TOK_FLOAT : TOK_INTEGER, sp,
                         (size_t)((l->src + l->pos) - sp), sl, sc);
}

static inline bool lexer_read_datetime(Lexer *l) {
  size_t sl = l->line, sc = l->col;
  const char *sp = l->src + l->pos;
  for (int i = 0; i < 4 && l->pos < l->src_len && is_ascii_digit(lexer_peek(l));
       i++)
    lexer_advance(l);
  if (lexer_peek(l) == '-') {
    lexer_advance(l);
    for (int i = 0;
         i < 2 && l->pos < l->src_len && is_ascii_digit(lexer_peek(l)); i++)
      lexer_advance(l);
    if (lexer_peek(l) == '-') {
      lexer_advance(l);
      for (int i = 0;
           i < 2 && l->pos < l->src_len && is_ascii_digit(lexer_peek(l)); i++)
        lexer_advance(l);
    }
  }
  if (lexer_peek(l) == 'T' || lexer_peek(l) == 't' || lexer_peek(l) == ' ') {
    lexer_advance(l);
    for (int i = 0;
         i < 2 && l->pos < l->src_len && is_ascii_digit(lexer_peek(l)); i++)
      lexer_advance(l);
    if (lexer_peek(l) == ':') {
      lexer_advance(l);
      for (int i = 0;
           i < 2 && l->pos < l->src_len && is_ascii_digit(lexer_peek(l)); i++)
        lexer_advance(l);
      if (lexer_peek(l) == ':') {
        lexer_advance(l);
        for (int i = 0;
             i < 2 && l->pos < l->src_len && is_ascii_digit(lexer_peek(l)); i++)
          lexer_advance(l);
        if (lexer_peek(l) == '.') {
          lexer_advance(l);
          while (l->pos < l->src_len && is_ascii_digit(lexer_peek(l)))
            lexer_advance(l);
        }
      }
    }
    if (lexer_peek(l) == 'Z' || lexer_peek(l) == 'z') {
      lexer_advance(l);
    } else if (lexer_peek(l) == '+' || lexer_peek(l) == '-') {
      lexer_advance(l);
      for (int i = 0;
           i < 2 && l->pos < l->src_len && is_ascii_digit(lexer_peek(l)); i++)
        lexer_advance(l);
      if (lexer_peek(l) == ':') {
        lexer_advance(l);
        for (int i = 0;
             i < 2 && l->pos < l->src_len && is_ascii_digit(lexer_peek(l)); i++)
          lexer_advance(l);
      }
    }
  }
  return lexer_add_token(l, TOK_DATETIME, sp, (size_t)((l->src + l->pos) - sp),
                         sl, sc);
}

static inline bool lexer_read_bare_key(Lexer *l) {
  size_t sl = l->line, sc = l->col;
  const char *sp = l->src + l->pos;
  while (l->pos < l->src_len && is_bare_key_char(lexer_peek(l)))
    lexer_advance(l);
  size_t len = (size_t)((l->src + l->pos) - sp);
  if (len == 0) {
    lexer_set_error(l, "expected key");
    return false;
  }
  return lexer_add_token(l, TOK_IDENT, sp, len, sl, sc);
}
static inline bool lexer_next_token(Lexer *l) {
  if (!lexer_skip_whitespace_and_comments(l))
    return false;
  if (l->pos >= l->src_len) {
    return lexer_add_token(l, TOK_EOF, l->src + l->pos, 0, l->line, l->col);
  }
  size_t sl = l->line, sc = l->col;
  char c = lexer_peek(l);
  if (c == '\n') {
    lexer_advance(l);
    return lexer_add_token(l, TOK_NEWLINE, l->src + l->pos - 1, 1, sl, sc);
  }
  if (c == '=') {
    lexer_advance(l);
    return lexer_add_token(l, TOK_EQUAL, l->src + l->pos - 1, 1, sl, sc);
  }
  if (c == '.') {
    lexer_advance(l);
    return lexer_add_token(l, TOK_DOT, l->src + l->pos - 1, 1, sl, sc);
  }
  if (c == ',') {
    lexer_advance(l);
    return lexer_add_token(l, TOK_COMMA, l->src + l->pos - 1, 1, sl, sc);
  }
  if (c == ':') {
    lexer_advance(l);
    return lexer_add_token(l, TOK_COLON, l->src + l->pos - 1, 1, sl, sc);
  }
  if (c == '{') {
    lexer_advance(l);
    return lexer_add_token(l, TOK_LBRACE, l->src + l->pos - 1, 1, sl, sc);
  }
  if (c == '}') {
    lexer_advance(l);
    return lexer_add_token(l, TOK_RBRACE, l->src + l->pos - 1, 1, sl, sc);
  }
  if (c == '[') {
    if (lexer_peek_at(l, 1) == '[') {
      lexer_advance(l);
      lexer_advance(l);
      return lexer_add_token(l, TOK_LLBRACKET, l->src + l->pos - 2, 2, sl, sc);
    }
    lexer_advance(l);
    return lexer_add_token(l, TOK_LBRACKET, l->src + l->pos - 1, 1, sl, sc);
  }
  if (c == ']') {
    if (lexer_peek_at(l, 1) == ']') {
      lexer_advance(l);
      lexer_advance(l);
      return lexer_add_token(l, TOK_RRBRACKET, l->src + l->pos - 2, 2, sl, sc);
    }
    lexer_advance(l);
    return lexer_add_token(l, TOK_RBRACKET, l->src + l->pos - 1, 1, sl, sc);
  }
  if (c == '"') {
    if (lexer_peek_at(l, 1) == '"' && lexer_peek_at(l, 2) == '"')
      return lexer_read_ml_basic_string(l);
    return lexer_read_string_basic(l);
  }
  if (c == '\'') {
    if (lexer_peek_at(l, 1) == '\'' && lexer_peek_at(l, 2) == '\'')
      return lexer_read_ml_literal_string(l);
    return lexer_read_string_literal(l);
  }
  if (is_ascii_digit(c)) {
    if (l->pos + 4 < l->src_len) {
      const char *p = l->src + l->pos;
      if (is_ascii_digit(p[0]) && is_ascii_digit(p[1]) &&
          is_ascii_digit(p[2]) && is_ascii_digit(p[3]) && p[4] == '-')
        return lexer_read_datetime(l);
    }
    return lexer_read_number(l);
  }
  if (c == '+' || c == '-') {
    if (l->pos + 1 < l->src_len && is_ascii_digit(lexer_peek_at(l, 1)))
      return lexer_read_number(l);
    if (l->pos + 3 < l->src_len) {
      const char *p = l->src + l->pos + 1;
      if ((p[0] == 'i' && p[1] == 'n' && p[2] == 'f') ||
          (p[0] == 'n' && p[1] == 'a' && p[2] == 'n'))
        return lexer_read_number(l);
    }
  }
  if (is_bare_key_char(c)) {
    if (lexer_match_keyword(l, "true", 4)) {
      for (int i = 0; i < 4; i++)
        lexer_advance(l);
      return lexer_add_token(l, TOK_BOOL, l->src + l->pos - 4, 4, sl, sc);
    }
    if (lexer_match_keyword(l, "false", 5)) {
      for (int i = 1; i < 5; i++)
        lexer_advance(l);
      return lexer_add_token(l, TOK_BOOL, l->src + l->pos - 5, 5, sl, sc);
    }
    return lexer_read_bare_key(l);
  }
  lexer_set_error(l, "unexpected character");
  return false;
}

bool lexer_tokenize(Lexer *l) {
  while (!l->has_error) {
    if (!lexer_next_token(l)) {
      if (l->has_error)
        return false;
    }
    break;
  }
  return !l->has_error;
}
