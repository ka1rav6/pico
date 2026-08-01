#pragma once

#include "parser.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace Pico::TOML {

class TomlError : public std::runtime_error {
public:
    explicit TomlError(const std::string &msg) : std::runtime_error(msg) {}
};

class Table;
class Node;

// High-level TOML document. Parses the source in the constructor and exposes
// ergonomic accessors over the underlying AST.
//
//   Pico::TOML::Toml toml(src);
//   if (static_cast<std::string>(toml.table("project").node("language")) ==
//       "cpp") { ... }
//
// Tables and Nodes returned here reference the document's AST, so the Toml
// must outlive them. Missing tables/nodes yield invalid Table/Node objects;
// converting an invalid or wrong-typed Node throws TomlError.
class Toml {
public:
    explicit Toml(const std::string &src);
    ~Toml();

    Toml(const Toml &) = delete;
    Toml &operator=(const Toml &) = delete;
    Toml(Toml &&) = delete;
    Toml &operator=(Toml &&) = delete;

    bool ok() const { return ok_; }
    const std::string &error_msg() const { return error_msg_; }
    size_t error_line() const { return error_line_; }
    size_t error_col() const { return error_col_; }

    // root-level key (a key/value written before any [table] header)
    Node node(const std::string &key) const;
    // named [table]
    Table table(const std::string &name) const;
    // all [[array-of-tables]] elements with the given name
    std::vector<Table> tables(const std::string &name) const;

private:
    bool ok_ = false;
    std::string error_msg_;
    size_t error_line_ = 0;
    size_t error_col_ = 0;
    Lexer lexer_;
    Parser parser_;
    Program program_;
};

// A [table] section (or sub-table).
class Table {
public:
    Table() = default;
    explicit Table(const TomlTable *t) : table_(t) {}

    bool valid() const { return table_ != nullptr; }

    // direct key or dotted path: node("debug.optimization")
    Node node(const std::string &key) const;
    // nested [sub.table]
    Table table(const std::string &name) const;
    bool contains(const std::string &key) const;

private:
    const TomlTable *table_ = nullptr;
};

// A TOML value. Converts to the underlying type with static_cast.
class Node {
public:
    Node() = default;
    explicit Node(const Value *v) : value_(v) {}

    bool valid() const { return value_ != nullptr; }

    bool is_string() const { return type() == ValType::VAL_STRING; }
    bool is_integer() const { return type() == ValType::VAL_INTEGER; }
    bool is_float() const { return type() == ValType::VAL_FLOAT; }
    bool is_bool() const { return type() == ValType::VAL_BOOL; }
    bool is_array() const { return type() == ValType::VAL_ARRAY; }
    bool is_table() const { return type() == ValType::VAL_INLINE_TABLE; }
    bool is_number() const { return is_integer() || is_float(); }

    std::string as_string() const;
    long as_integer() const;
    double as_float() const;
    bool as_bool() const;
    Table as_table() const;

    operator std::string() const { return as_string(); }
    operator long() const { return as_integer(); }
    operator double() const { return as_float(); }
    operator bool() const { return as_bool(); }

    size_t size() const;
    Node operator[](size_t i) const;

private:
    ValType type() const {
        if (!value_)
            throw TomlError("access on missing TOML node");
        return value_->type;
    }
    const Value *value_ = nullptr;
};

} // namespace Pico::TOML
