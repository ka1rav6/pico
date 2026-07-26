#ifndef PICO_TOML_PARSER_H
#define PICO_TOML_PARSER_H

#include "lexer.h"
#include "token.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// value types that a TOML key can hold
typedef enum {
  VAL_INTEGER,
  VAL_FLOAT,
  VAL_STRING,
  VAL_BOOL,
  VAL_DATETIME,
  VAL_ARRAY,
  VAL_INLINE_TABLE,
} ValType;

// forward declaration
typedef struct Table Table;

// a TOML value — holds any of the supported types
typedef struct Value {
  ValType type;
  union {
    long integer;
    double float_val;
    char *string;
    bool boolean;
    char *datetime;
    struct {
      struct Value *items;
      size_t len;
    } array;
    Table *inline_table;
  } as;
} Value;

// a key can be dotted: "database.host" -> segments = ["database", "host"]
typedef struct {
  char **segments;
  size_t segment_count;
} Key;

// a single key = value entry
typedef struct {
  Key key;
  Value value;
} Entry;

// a table is a [header] section containing entries and optional child tables
struct Table {
  Entry *entries;
  size_t entry_count;
  size_t entry_capacity;

  Table **children;
  size_t child_count;
  size_t child_capacity;

  char *name;
  bool is_array; // true for [[array_of_tables]]
};

// the root program — holds the implicit root table and all named tables
typedef struct {
  Table root;
  Table **all_tables;
  size_t table_count;
} Program;

// parser state
typedef struct {
  const Lexer *lexer;
  size_t pos;
  bool has_error;
  char error_msg[256];
  size_t error_line;
  size_t error_col;
} Parser;

// ast construction
Value value_integer(long v);
Value value_float(double v);
Value value_string(char *v);
Value value_bool(bool v);
Value value_datetime(char *v);
Value value_array(Value *items, size_t len);
Value value_inline_table(Table *t);

Key key_single(char *segment);
Key key_dotted(char **segments, size_t count);

void entry_init(Entry *e);
void entry_set_key(Entry *e, Key k);
void entry_set_value(Entry *e, Value v);

void table_init(Table *t);
void table_add_entry(Table *t, Entry e);
void table_add_child(Table *t, Table *child);

Program *program_init(void);
void program_add_table(Program *p, Table *t);

// parser
Parser parser_init(const Lexer *lexer);
bool parser_parse(Parser *p, Program *out);

#endif // PICO_TOML_PARSER_H
