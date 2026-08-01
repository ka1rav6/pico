#include "toml.hpp"

namespace Pico::TOML {

Toml::Toml(const std::string &src) : lexer_(src), parser_(lexer_) {
    if (!lexer_.Lexer_tokenize()) {
        ok_ = false;
        error_msg_ = lexer_.error_msg;
        error_line_ = lexer_.error_line;
        error_col_ = lexer_.error_col;
        return;
    }
    if (!parser_.parse(&program_)) {
        ok_ = false;
        error_msg_ = parser_.error_msg;
        error_line_ = parser_.error_line;
        error_col_ = parser_.error_col;
        return;
    }
    ok_ = true;
}

Toml::~Toml() = default;

Node Toml::node(const std::string &key) const {
    return Table(&program_.root).node(key);
}

Table Toml::table(const std::string &name) const {
    for (TomlTable *c : program_.root.children)
        if (!c->name.empty() && c->name == name)
            return Table(c);
    return Table();
}

std::vector<Table> Toml::tables(const std::string &name) const {
    std::vector<Table> out;
    for (TomlTable *c : program_.root.children)
        if (!c->name.empty() && c->name == name)
            out.push_back(Table(c));
    return out;
}

Node Table::node(const std::string &key) const {
    if (!table_)
        return Node();

    size_t dot = key.find('.');
    if (dot != std::string::npos)
        return table(key.substr(0, dot)).node(key.substr(dot + 1));

    for (const Entry &e : table_->entries)
        if (!e.key.segments.empty() && e.key.segments[0] == key)
            return Node(&e.value);
    return Node();
}

Table Table::table(const std::string &name) const {
    if (!table_)
        return Table();
    for (TomlTable *c : table_->children)
        if (!c->name.empty() && c->name == name)
            return Table(c);
    return Table();
}

bool Table::contains(const std::string &key) const { return node(key).valid(); }

std::string Node::as_string() const {
    if (!value_ || value_->type != ValType::VAL_STRING)
        throw TomlError("expected TOML string");
    return std::get<std::string>(value_->as);
}

long Node::as_integer() const {
    if (!value_)
        throw TomlError("expected TOML integer");
    if (value_->type == ValType::VAL_FLOAT)
        return static_cast<long>(std::get<double>(value_->as));
    if (value_->type != ValType::VAL_INTEGER)
        throw TomlError("expected TOML integer");
    return std::get<long>(value_->as);
}

double Node::as_float() const {
    if (!value_)
        throw TomlError("expected TOML float");
    if (value_->type == ValType::VAL_INTEGER)
        return static_cast<double>(std::get<long>(value_->as));
    if (value_->type != ValType::VAL_FLOAT)
        throw TomlError("expected TOML float");
    return std::get<double>(value_->as);
}

bool Node::as_bool() const {
    if (!value_ || value_->type != ValType::VAL_BOOL)
        throw TomlError("expected TOML bool");
    return std::get<bool>(value_->as);
}

Table Node::as_table() const {
    if (!value_ || value_->type != ValType::VAL_INLINE_TABLE)
        throw TomlError("expected TOML table");
    return Table(std::get<TomlTable *>(value_->as));
}

size_t Node::size() const {
    if (!value_ || value_->type != ValType::VAL_ARRAY)
        throw TomlError("expected TOML array");
    return std::get<std::vector<Value>>(value_->as).size();
}

Node Node::operator[](size_t i) const {
    if (!value_ || value_->type != ValType::VAL_ARRAY)
        throw TomlError("expected TOML array");
    const std::vector<Value> &arr = std::get<std::vector<Value>>(value_->as);
    if (i >= arr.size())
        throw TomlError("array index out of bounds");
    return Node(&arr[i]);
}

} // namespace Pico::TOML
