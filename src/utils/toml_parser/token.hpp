#pragma once

#include <iostream>
#include <stdint.h>
#include <string>

namespace Pico::TOML {
enum class TokenType : uint8_t {
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
};

static const std::string token_type_name(TokenType t) {
    switch (t) {
    case TokenType::TOK_INTEGER:
        return "INTEGER";
    case TokenType::TOK_FLOAT:
        return "FLOAT";
    case TokenType::TOK_STRING:
        return "STRING";
    case TokenType::TOK_BOOL:
        return "BOOL";
    case TokenType::TOK_DATETIME:
        return "DATETIME";
    case TokenType::TOK_IDENT:
        return "IDENT";
    case TokenType::TOK_EQUAL:
        return "EQUAL";
    case TokenType::TOK_DOT:
        return "DOT";
    case TokenType::TOK_COMMA:
        return "COMMA";
    case TokenType::TOK_COLON:
        return "COLON";
    case TokenType::TOK_LBRACKET:
        return "LBRACKET";
    case TokenType::TOK_RBRACKET:
        return "RBRACKET";
    case TokenType::TOK_LLBRACKET:
        return "LLBRACKET";
    case TokenType::TOK_RRBRACKET:
        return "RRBRACKET";
    case TokenType::TOK_LBRACE:
        return "LBRACE";
    case TokenType::TOK_RBRACE:
        return "RBRACE";
    case TokenType::TOK_NEWLINE:
        return "NEWLINE";
    case TokenType::TOK_EOF:
        return "EOF";
    }
    return "UNKNOWN";
}

class Token {
public:
    TokenType type;
    const char *start;
    size_t len;
    size_t line;
    size_t col;
    Token(TokenType type, const char *start, size_t length, size_t line,
          size_t col) {
        this->type = type;
        this->start = start;
        this->len = length;
        this->line = line;
        this->col = col;
    }
};
} // namespace Pico::TOML
