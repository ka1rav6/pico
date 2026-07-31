#pragma once

#include "token.hpp"
#include <string>
#include <vector>

namespace Pico::TOML {
class Lexer {
public:
    std::string src;
    size_t pos;
    size_t line;
    size_t col;
    std::vector<Token *> tokens;
    bool has_error;
    std::string error_msg;
    size_t error_line;
    size_t error_col;
    Lexer(const std::string src);
    ~Lexer();
    bool Lexer_tokenize();

private:
    inline char peek();
    inline char peek_at(size_t offset);
    inline char advance();
    inline bool match(const char expected);
    inline void set_error(const std::string msg);
    inline bool add_token(TokenType, const char *start, size_t length);
    inline bool is_ascii_alpha(char c);
    inline bool is_ascii_digit(char c);
    inline bool is_ascii_hex_digit(char c);
    inline bool is_bare_key_char(char c);
    inline bool skip_whitespaces_and_comments();
    inline bool match_keyword(const std::string kw);
    inline bool read_string_basic();
    inline bool read_string_literal();
    inline bool read_ml_basic_string();
    inline bool read_ml_literal_string();
    inline bool read_number();
    inline bool read_datetime();
    inline bool read_bare_key();
    inline bool next_token();
};
} // namespace Pico::TOML
