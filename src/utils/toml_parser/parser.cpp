#include "parser.hpp"

#include <cstdlib>
#include <cstring>
#include <utility>

namespace Pico::TOML {

Value value_integer(long v) {
    Value val;
    val.type = ValType::VAL_INTEGER;
    val.as = v;
    return val;
}

Value value_float(double v) {
    Value val;
    val.type = ValType::VAL_FLOAT;
    val.as = v;
    return val;
}

Value value_string(const std::string &v) {
    Value val;
    val.type = ValType::VAL_STRING;
    val.as = v;
    return val;
}

Value value_bool(bool v) {
    Value val;
    val.type = ValType::VAL_BOOL;
    val.as = v;
    return val;
}

Value value_datetime(const std::string &v) {
    Value val;
    val.type = ValType::VAL_DATETIME;
    val.as = v;
    return val;
}

Value value_array(std::vector<Value> items) {
    Value val;
    val.type = ValType::VAL_ARRAY;
    val.as = std::move(items);
    return val;
}

Value value_inline_table(TomlTable *t) {
    Value val;
    val.type = ValType::VAL_INLINE_TABLE;
    val.as = t;
    return val;
}

Parser::Parser(const Lexer &lexer) {
    this->lexer = &lexer;
    this->pos = 0;
    this->has_error = false;
    this->error_msg.clear();
    this->error_line = 0;
    this->error_col = 0;
}

const Token *Parser::peek() {
    if (pos >= lexer->tokens.size())
        return lexer->tokens.back();
    return lexer->tokens[pos];
}

const Token *Parser::advance() {
    const Token *t = peek();
    if (t->type != TokenType::TOK_EOF)
        pos++;
    return t;
}

bool Parser::expect(TokenType type, const std::string &msg) {
    if (peek()->type != type) {
        const Token *t = peek();
        has_error = true;
        error_msg = msg + " at line " + std::to_string(t->line);
        error_line = t->line;
        error_col = t->col;
        return false;
    }
    advance();
    return true;
}

void Parser::skip_newlines() {
    while (peek()->type == TokenType::TOK_NEWLINE)
        advance();
}

std::string Parser::token_to_string(const Token *t) const {
    return std::string(t->start, t->len);
}

std::string Parser::token_string_unquote(const Token *t) const {
    const char *start = t->start;
    size_t len = t->len;
    if ((start[0] == '"' && start[len - 1] == '"') ||
        (start[0] == '\'' && start[len - 1] == '\'')) {
        return std::string(start + 1, len - 2);
    }
    return std::string(start, len);
}

TomlTable *Parser::find_or_create_child(TomlTable *parent,
                                        const std::string &name,
                                        bool is_array) {
    for (TomlTable *child : parent->children) {
        if (!child->name.empty() && child->name == name &&
            child->is_array == is_array) {
            return child;
        }
    }
    TomlTable *child = new TomlTable;
    child->name = name;
    child->is_array = is_array;
    parent->children.push_back(child);
    return child;
}

bool Parser::parse_value(Value *out, Program *prog) {
    const Token *t = peek();

    switch (t->type) {
    case TokenType::TOK_INTEGER: {
        advance();
        std::string s = token_to_string(t);
        *out = value_integer(std::strtol(s.c_str(), nullptr, 0));
        return true;
    }
    case TokenType::TOK_FLOAT: {
        advance();
        std::string s = token_to_string(t);
        *out = value_float(std::strtod(s.c_str(), nullptr));
        return true;
    }
    case TokenType::TOK_STRING: {
        advance();
        *out = value_string(token_string_unquote(t));
        return true;
    }
    case TokenType::TOK_BOOL: {
        advance();
        bool v = (t->len == 4 && std::strncmp(t->start, "true", 4) == 0);
        *out = value_bool(v);
        return true;
    }
    case TokenType::TOK_DATETIME: {
        advance();
        *out = value_datetime(token_to_string(t));
        return true;
    }
    case TokenType::TOK_LBRACKET:
        return parse_array(out, prog);
    case TokenType::TOK_LBRACE: {
        TomlTable *tbl = new TomlTable;
        if (!parse_inline_table_content(tbl, prog)) {
            delete tbl;
            return false;
        }
        prog->all_tables.push_back(tbl);
        *out = value_inline_table(tbl);
        return true;
    }
    default: {
        has_error = true;
        error_msg = "unexpected token '" + std::string(t->start, t->len) +
                    "' at line " + std::to_string(t->line);
        error_line = t->line;
        error_col = t->col;
        return false;
    }
    }
}

bool Parser::parse_array(Value *out, Program *prog) {
    if (!expect(TokenType::TOK_LBRACKET, "expected '['"))
        return false;

    skip_newlines();
    if (peek()->type == TokenType::TOK_RBRACKET) {
        advance();
        *out = value_array({});
        return true;
    }

    std::vector<Value> items;
    Value item;
    if (!parse_value(&item, prog))
        return false;
    items.push_back(item);
    skip_newlines();

    while (peek()->type == TokenType::TOK_COMMA) {
        advance();
        skip_newlines();
        if (peek()->type == TokenType::TOK_RBRACKET)
            break;
        if (!parse_value(&item, prog))
            return false;
        items.push_back(item);
        skip_newlines();
    }

    if (!expect(TokenType::TOK_RBRACKET, "expected ']'"))
        return false;

    *out = value_array(std::move(items));
    return true;
}

bool Parser::parse_inline_table_content(TomlTable *out, Program *prog) {
    if (!expect(TokenType::TOK_LBRACE, "expected '{'"))
        return false;

    if (peek()->type == TokenType::TOK_RBRACE) {
        advance();
        return true;
    }

    Entry e;
    if (!parse_entry(&e, prog))
        return false;
    out->entries.push_back(e);

    while (peek()->type == TokenType::TOK_COMMA) {
        advance();
        if (peek()->type == TokenType::TOK_RBRACE)
            break;
        if (!parse_entry(&e, prog))
            return false;
        out->entries.push_back(e);
    }

    if (!expect(TokenType::TOK_RBRACE, "expected '}'"))
        return false;

    return true;
}

bool Parser::parse_key(Key *out) {
    const Token *first = peek();
    if (first->type != TokenType::TOK_IDENT &&
        first->type != TokenType::TOK_STRING) {
        has_error = true;
        error_msg = "expected key at line " + std::to_string(first->line);
        error_line = first->line;
        error_col = first->col;
        return false;
    }

    out->segments.clear();
    out->segments.push_back(token_to_string(advance()));

    while (peek()->type == TokenType::TOK_DOT) {
        advance();
        const Token *next = peek();
        if (next->type != TokenType::TOK_IDENT &&
            next->type != TokenType::TOK_STRING) {
            has_error = true;
            error_msg =
                "expected key after '.' at line " + std::to_string(next->line);
            error_line = next->line;
            error_col = next->col;
            return false;
        }
        out->segments.push_back(token_to_string(advance()));
    }

    return true;
}

bool Parser::parse_entry(Entry *out, Program *prog) {
    Key k;
    if (!parse_key(&k))
        return false;
    if (!expect(TokenType::TOK_EQUAL, "expected '='"))
        return false;
    Value v;
    if (!parse_value(&v, prog))
        return false;
    out->key = std::move(k);
    out->value = std::move(v);
    return true;
}

bool Parser::parse_table_header(Program *prog, bool is_array) {
    advance(); // consume [ or [[

    Key key;
    if (!parse_key(&key))
        return false;

    if (is_array) {
        if (!expect(TokenType::TOK_RRBRACKET, "expected ']]'"))
            return false;
    } else {
        if (!expect(TokenType::TOK_RBRACKET, "expected ']'"))
            return false;
    }

    TomlTable *current = &prog->root;
    for (size_t i = 0; i < key.segments.size(); i++) {
        bool last = (i == key.segments.size() - 1);
        if (last && is_array) {
            TomlTable *new_table = new TomlTable;
            new_table->name = key.segments[i];
            new_table->is_array = true;
            current->children.push_back(new_table);
            prog->all_tables.push_back(new_table);
            current = new_table;
        } else {
            TomlTable *child =
                find_or_create_child(current, key.segments[i], false);
            if (last) {
                child->name = key.segments[i];
                bool known = false;
                for (TomlTable *t : prog->all_tables)
                    if (t == child)
                        known = true;
                if (!known)
                    prog->all_tables.push_back(child);
            }
            current = child;
        }
    }

    return true;
}

bool Parser::parse(Program *out) {
    skip_newlines();

    TomlTable *current = &out->root;

    while (peek()->type != TokenType::TOK_EOF && !has_error) {
        const Token *t = peek();

        if (t->type == TokenType::TOK_LLBRACKET) {
            if (!parse_table_header(out, true))
                return false;
            current = out->all_tables.back();
        } else if (t->type == TokenType::TOK_LBRACKET) {
            if (!parse_table_header(out, false))
                return false;
            current = out->all_tables.back();
        } else {
            Entry e;
            if (!parse_entry(&e, out))
                return false;
            current->entries.push_back(e);
        }

        skip_newlines();
    }

    return !has_error;
}

Program::~Program() {
    for (TomlTable *t : all_tables)
        delete t;
}

} // namespace Pico::TOML
