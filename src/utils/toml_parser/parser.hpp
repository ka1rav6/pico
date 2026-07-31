#pragma once

#include "lexer.hpp"

#include <string>
#include <variant>
#include <vector>

namespace Pico::TOML {

// value types that a TOML key can hold
enum class ValType {
    VAL_INTEGER,
    VAL_FLOAT,
    VAL_STRING,
    VAL_BOOL,
    VAL_DATETIME,
    VAL_ARRAY,
    VAL_INLINE_TABLE,
};

struct Table;

// a TOML value - holds any of the supported types
struct Value {
    ValType type;
    std::variant<long, double, std::string, bool, std::vector<Value>, Table *>
        as;
};

// ast construction
Value value_integer(long v);
Value value_float(double v);
Value value_string(const std::string &v);
Value value_bool(bool v);
Value value_datetime(const std::string &v);
Value value_array(std::vector<Value> items);
Value value_inline_table(Table *t);

// a key can be dotted: "database.host" -> segments = ["database", "host"]
struct Key {
    std::vector<std::string> segments;
};

// a single key = value entry
struct Entry {
    Key key;
    Value value;
};

// a table is a [header] section containing entries and optional child tables
struct Table {
    std::vector<Entry> entries;
    std::vector<Table *> children;
    std::string name;
    bool is_array = false; // true for [[array_of_tables]]
};

// the root program - holds the implicit root table and all named tables
struct Program {
    Table root;
    std::vector<Table *> all_tables;
};

// parser state
class Parser {
public:
    const Lexer *lexer;
    size_t pos = 0;
    bool has_error = false;
    std::string error_msg;
    size_t error_line = 0;
    size_t error_col = 0;

    Parser(const Lexer &lexer);
    bool parse(Program *out);

private:
    const Token *peek();
    const Token *advance();
    bool expect(TokenType type, const std::string &msg);
    void skip_newlines();
    std::string token_to_string(const Token *t) const;
    std::string token_string_unquote(const Token *t) const;
    Table *find_or_create_child(Table *parent, const std::string &name,
                                bool is_array);
    bool parse_value(Value *out);
    bool parse_array(Value *out);
    bool parse_inline_table_content(Table *out);
    bool parse_key(Key *out);
    bool parse_entry(Entry *out);
    bool parse_table_header(Program *prog, bool is_array);
};

} // namespace Pico::TOML
