
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Value value_integer(long v) {
  Value val;
  val.type = VAL_INTEGER;
  val.as.integer = v;
  return val;
}

Value value_float(double v) {
  Value val;
  val.type = VAL_FLOAT;
  val.as.float_val = v;
  return val;
}

Value value_string(char *v) {
  Value val;
  val.type = VAL_STRING;
  val.as.string = v;
  return val;
}

Value value_bool(bool v) {
  Value val;
  val.type = VAL_BOOL;
  val.as.boolean = v;
  return val;
}

Value value_datetime(char *v) {
  Value val;
  val.type = VAL_DATETIME;
  val.as.datetime = v;
  return val;
}

Value value_array(Value *items, size_t len) {
  Value val;
  val.type = VAL_ARRAY;
  val.as.array.items = items;
  val.as.array.len = len;
  return val;
}

Value value_inline_table(Table *t) {
  Value val;
  val.type = VAL_INLINE_TABLE;
  val.as.inline_table = t;
  return val;
}

Key key_single(char *segment) {
  Key k;
  k.segments = (char **)malloc(sizeof(char *));
  k.segments[0] = segment;
  k.segment_count = 1;
  return k;
}

Key key_dotted(char **segments, size_t count) {
  Key k;
  k.segments = segments;
  k.segment_count = count;
  return k;
}

void entry_init(Entry *e) {
  e->key.segments = NULL;
  e->key.segment_count = 0;
  e->value.type = VAL_STRING;
  e->value.as.string = NULL;
}

void entry_set_key(Entry *e, Key k) { e->key = k; }

void entry_set_value(Entry *e, Value v) { e->value = v; }

void table_init(Table *t) {
  t->entries = NULL;
  t->entry_count = 0;
  t->entry_capacity = 0;
  t->children = NULL;
  t->child_count = 0;
  t->child_capacity = 0;
  t->name = NULL;
  t->is_array = false;
}

void table_add_entry(Table *t, Entry e) {
  if (t->entry_count >= t->entry_capacity) {
    t->entry_capacity = t->entry_capacity == 0 ? 8 : t->entry_capacity * 2;
    t->entries = (Entry *)realloc(t->entries, sizeof(Entry) * t->entry_capacity);
  }
  t->entries[t->entry_count++] = e;
}

void table_add_child(Table *t, Table *child) {
  if (t->child_count >= t->child_capacity) {
    t->child_capacity = t->child_capacity == 0 ? 8 : t->child_capacity * 2;
    t->children =
        (Table **)realloc(t->children, sizeof(Table *) * t->child_capacity);
  }
  t->children[t->child_count++] = child;
}

Program *program_init(void) {
  Program *p = (Program *)malloc(sizeof(Program));
  table_init(&p->root);
  p->all_tables = NULL;
  p->table_count = 0;
  return p;
}

void program_add_table(Program *p, Table *t) {
  p->table_count++;
  p->all_tables =
      (Table **)realloc(p->all_tables, sizeof(Table *) * p->table_count);
  p->all_tables[p->table_count - 1] = t;
}

Parser parser_init(const Lexer *lexer) {
  Parser p;
  p.lexer = lexer;
  p.pos = 0;
  p.has_error = false;
  p.error_msg[0] = '\0';
  p.error_line = 0;
  p.error_col = 0;
  return p;
}

static inline const Token *parser_peek(const Parser *p) {
  if (p->pos >= p->lexer->token_count)
    return &p->lexer->tokens[p->lexer->token_count - 1];
  return &p->lexer->tokens[p->pos];
}

static inline const Token *parser_advance(Parser *p) {
  const Token *t = parser_peek(p);
  if (t->type != TOK_EOF)
    p->pos++;
  return t;
}

static inline bool parser_expect(Parser *p, TokenType type, const char *msg) {
  if (parser_peek(p)->type != type) {
    const Token *t = parser_peek(p);
    p->has_error = true;
    snprintf(p->error_msg, sizeof(p->error_msg), "%s at line %zu", msg,
             t->line);
    p->error_line = t->line;
    p->error_col = t->col;
    return false;
  }
  parser_advance(p);
  return true;
}

static inline void parser_skip_newlines(Parser *p) {
  while (parser_peek(p)->type == TOK_NEWLINE)
    parser_advance(p);
}

static char *token_to_cstring(const Token *t) {
  size_t len = t->length;
  char *s = (char *)malloc(len + 1);
  memcpy(s, t->start, len);
  s[len] = '\0';
  return s;
}

static char *token_string_unquote(const Token *t) {
  const char *start = t->start;
  size_t len = t->length;
  if ((start[0] == '"' && start[len - 1] == '"') ||
      (start[0] == '\'' && start[len - 1] == '\'')) {
    size_t inner_len = len - 2;
    char *s = (char *)malloc(inner_len + 1);
    memcpy(s, start + 1, inner_len);
    s[inner_len] = '\0';
    return s;
  }
  return token_to_cstring(t);
}

static Table *table_find_or_create_child(Table *parent, const char *name,
                                         bool is_array) {
  for (size_t i = 0; i < parent->child_count; i++) {
    if (parent->children[i]->name &&
        strcmp(parent->children[i]->name, name) == 0 &&
        parent->children[i]->is_array == is_array) {
      return parent->children[i];
    }
  }
  Table *child = (Table *)malloc(sizeof(Table));
  table_init(child);
  child->name = strdup(name);
  child->is_array = is_array;
  table_add_child(parent, child);
  return child;
}

static bool parse_value(Parser *p, Value *out);
static bool parse_entry(Parser *p, Entry *out);
static bool parse_inline_table_content(Parser *p, Table *out);
static bool parse_array(Parser *p, Value *out);

static bool parse_value(Parser *p, Value *out) {
  const Token *t = parser_peek(p);

  switch (t->type) {
  case TOK_INTEGER: {
    parser_advance(p);
    char *s = token_to_cstring(t);
    char *end;
    long v = strtol(s, &end, 0);
    free(s);
    *out = value_integer(v);
    return true;
  }
  case TOK_FLOAT: {
    parser_advance(p);
    char *s = token_to_cstring(t);
    char *end;
    double v = strtod(s, &end);
    free(s);
    *out = value_float(v);
    return true;
  }
  case TOK_STRING: {
    parser_advance(p);
    char *v = token_string_unquote(t);
    *out = value_string(v);
    return true;
  }
  case TOK_BOOL: {
    parser_advance(p);
    bool v = (t->length == 4 && strncmp(t->start, "true", 4) == 0);
    *out = value_bool(v);
    return true;
  }
  case TOK_DATETIME: {
    parser_advance(p);
    char *v = token_to_cstring(t);
    *out = value_datetime(v);
    return true;
  }
  case TOK_LBRACKET:
    return parse_array(p, out);
  case TOK_LBRACE: {
    Table *tbl = (Table *)malloc(sizeof(Table));
    table_init(tbl);
    if (!parse_inline_table_content(p, tbl)) {
      free(tbl);
      return false;
    }
    *out = value_inline_table(tbl);
    return true;
  }
  default:
    p->has_error = true;
    snprintf(p->error_msg, sizeof(p->error_msg),
             "unexpected token '%.*s' at line %zu", (int)t->length, t->start,
             t->line);
    p->error_line = t->line;
    p->error_col = t->col;
    return false;
  }
}

static bool parse_array(Parser *p, Value *out) {
  if (!parser_expect(p, TOK_LBRACKET, "expected '['"))
    return false;

  if (parser_peek(p)->type == TOK_RBRACKET) {
    parser_advance(p);
    *out = value_array(NULL, 0);
    return true;
  }

  size_t capacity = 8;
  size_t len = 0;
  Value *items = (Value *)malloc(sizeof(Value) * capacity);

  Value item;
  if (!parse_value(p, &item)) {
    free(items);
    return false;
  }
  items[len++] = item;

  while (parser_peek(p)->type == TOK_COMMA) {
    parser_advance(p);
    if (parser_peek(p)->type == TOK_RBRACKET)
      break;
    if (!parse_value(p, &item)) {
      free(items);
      return false;
    }
    if (len >= capacity) {
      capacity *= 2;
      items = (Value *)realloc(items, sizeof(Value) * capacity);
    }
    items[len++] = item;
  }

  if (!parser_expect(p, TOK_RBRACKET, "expected ']'")) {
    free(items);
    return false;
  }

  *out = value_array(items, len);
  return true;
}

static bool parse_inline_table_content(Parser *p, Table *out) {
  if (!parser_expect(p, TOK_LBRACE, "expected '{'"))
    return false;

  if (parser_peek(p)->type == TOK_RBRACE) {
    parser_advance(p);
    return true;
  }

  Entry e;
  if (!parse_entry(p, &e))
    return false;
  table_add_entry(out, e);

  while (parser_peek(p)->type == TOK_COMMA) {
    parser_advance(p);
    if (parser_peek(p)->type == TOK_RBRACE)
      break;
    if (!parse_entry(p, &e))
      return false;
    table_add_entry(out, e);
  }

  if (!parser_expect(p, TOK_RBRACE, "expected '}'"))
    return false;

  return true;
}

static bool parse_key(Parser *p, Key *out) {
  const Token *first = parser_peek(p);
  if (first->type != TOK_IDENT && first->type != TOK_STRING) {
    p->has_error = true;
    snprintf(p->error_msg, sizeof(p->error_msg), "expected key at line %zu",
             first->line);
    p->error_line = first->line;
    p->error_col = first->col;
    return false;
  }

  size_t capacity = 4;
  size_t count = 0;
  char **segments = (char **)malloc(sizeof(char *) * capacity);

  segments[count++] = token_to_cstring(parser_advance(p));

  while (parser_peek(p)->type == TOK_DOT) {
    parser_advance(p);
    const Token *next = parser_peek(p);
    if (next->type != TOK_IDENT && next->type != TOK_STRING) {
      p->has_error = true;
      snprintf(p->error_msg, sizeof(p->error_msg),
               "expected key after '.' at line %zu", next->line);
      p->error_line = next->line;
      p->error_col = next->col;
      for (size_t i = 0; i < count; i++)
        free(segments[i]);
      free(segments);
      return false;
    }
    if (count >= capacity) {
      capacity *= 2;
      segments = (char **)realloc(segments, sizeof(char *) * capacity);
    }
    segments[count++] = token_to_cstring(parser_advance(p));
  }

  if (count == 1) {
    *out = key_single(segments[0]);
    free(segments);
  } else {
    *out = key_dotted(segments, count);
  }
  return true;
}

static bool parse_entry(Parser *p, Entry *out) {
  entry_init(out);
  Key k;
  if (!parse_key(p, &k))
    return false;
  if (!parser_expect(p, TOK_EQUAL, "expected '='"))
    return false;
  Value v;
  if (!parse_value(p, &v))
    return false;
  entry_set_key(out, k);
  entry_set_value(out, v);
  return true;
}

static bool parse_table_header(Parser *p, Program *prog, bool is_array) {
  parser_advance(p); // consume [ or [[

  Key key;
  if (!parse_key(p, &key))
    return false;

  if (is_array) {
    if (!parser_expect(p, TOK_RRBRACKET, "expected ']]'"))
      return false;
  } else {
    if (!parser_expect(p, TOK_RBRACKET, "expected ']'"))
      return false;
  }

  Table *current = &prog->root;
  for (size_t i = 0; i < key.segment_count; i++) {
    bool last = (i == key.segment_count - 1);
    if (last && is_array) {
      Table *new_table = (Table *)malloc(sizeof(Table));
      table_init(new_table);
      new_table->name = strdup(key.segments[i]);
      new_table->is_array = true;
      table_add_child(current, new_table);
      program_add_table(prog, new_table);
      current = new_table;
    } else {
      Table *child =
          table_find_or_create_child(current, key.segments[i], false);
      if (last) {
        child->name = strdup(key.segments[i]);
        program_add_table(prog, child);
      }
      current = child;
    }
  }

  return true;
}

bool parser_parse(Parser *p, Program *out) {
  parser_skip_newlines(p);

  while (parser_peek(p)->type != TOK_EOF && !p->has_error) {
    const Token *t = parser_peek(p);

    if (t->type == TOK_LLBRACKET) {
      if (!parse_table_header(p, out, true))
        return false;
    } else if (t->type == TOK_LBRACKET) {
      if (!parse_table_header(p, out, false))
        return false;
    } else {
      Entry e;
      if (!parse_entry(p, &e))
        return false;
      table_add_entry(&out->root, e);
    }

    parser_skip_newlines(p);
  }

  return !p->has_error;
}
