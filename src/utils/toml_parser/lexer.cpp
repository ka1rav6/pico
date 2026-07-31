#include "lexer.hpp"
#include "token.hpp"

namespace Pico::TOML {
Lexer::Lexer(const std::string src) {
    this->src = std::move(src);
    this->pos = 0;
    this->col = 1;
    this->line = 1;
    this->has_error = false;
    this->error_msg.clear();
    this->error_line = 0;
    this->error_col = 0;
}

Lexer::~Lexer() {
    for (Token *t : tokens)
        delete t;
}

inline char Lexer::peek() {
    if (pos >= src.length())
        return '\0';
    return this->src[this->pos];
}

inline char Lexer::peek_at(size_t offset) {
    size_t idx = pos + offset;
    if (idx >= src.length())
        return '\0';
    return src[idx];
}
inline char Lexer::advance() {
    char c = peek();
    if (c == '\0')
        return c;
    pos++;
    if (c == '\n') {
        line++;
        col = 1;
    } else
        col++;
    return c;
}
inline bool Lexer::match(const char expected) {
    if (peek() != expected)
        return false;
    advance();
    return true;
}
inline void Lexer::set_error(const std::string msg) {
    has_error = true;
    error_msg = std::move(msg);
    error_line = line;
    error_col = col;
}

inline bool Lexer::add_token(TokenType type, const char *start, size_t length) {
    Token *t = new Token(type, start, length, line, col);
    tokens.emplace_back(t);
    return true;
}
inline bool Lexer::is_ascii_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
inline bool Lexer::is_ascii_digit(char c) { return c >= '0' && c <= '9'; }

inline bool Lexer::is_ascii_hex_digit(char c) {
    return is_ascii_digit(c) || (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

inline bool Lexer::is_bare_key_char(char c) {
    return is_ascii_alpha(c) || is_ascii_digit(c) || c == '-' || c == '_';
}
inline bool Lexer::skip_whitespaces_and_comments() {
    while (pos < src.length()) {
        char c = peek();
        if (c == ' ' || c == '\t')
            advance();
        else if (c == '#') {
            while (pos < src.length() && peek() != '\n')
                advance();
        } else
            break;
    }
    return !has_error;
}

inline bool Lexer::match_keyword(const std::string kw) {
    if (static_cast<size_t>(src.length() - pos) < kw.length())
        return false;
    return src.compare(pos, kw.length(), kw) == 0 &&
           !is_bare_key_char(src[pos + kw.length()]);
}
inline bool Lexer::read_string_basic() {
    const char *sp = src.c_str() + pos;
    advance();
    while (pos < src.length()) {
        char c = peek();
        if (c == '"') {
            const char *end = src.c_str() + pos;
            advance();
            return add_token(TokenType::TOK_STRING, sp,
                             static_cast<size_t>(end - sp) + 1);
        } else if (c == '\\') {
            advance();
            if (pos < src.length()) {
                char esc = advance();
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
                    for (int i = 0; i < 4 && pos < src.length(); i++) {
                        if (!is_ascii_hex_digit(advance())) {
                            set_error("invalid \\u escape");
                            return false;
                        }
                    }
                    break;
                case 'U':
                    for (int i = 0; i < 8 && pos < src.length(); i++) {
                        if (!is_ascii_hex_digit(advance())) {
                            set_error("invalid \\U escape");
                            return false;
                        }
                    }
                    break;
                default:
                    set_error("invalid escape character");
                    return false;
                }
            }
        } else if (c == '\n') {
            set_error("unterminated basic string");
            return false;
        } else
            advance();
    }
    set_error("unterminated basic string");
    return false;
}
inline bool Lexer::read_string_literal() {
    const char *sp = src.c_str() + pos;
    advance();
    while (pos < src.length()) {
        char c = peek();
        if (c == '\'') {
            const char *end = src.c_str() + pos;
            advance();
            return add_token(TokenType::TOK_STRING, sp,
                             static_cast<size_t>(end - sp) + 1);
        } else if (c == '\n') {
            set_error("unterminated literal string");
            return false;
        } else {
            advance();
        }
    }
    set_error("unterminated literal string");
    return false;
}

inline bool Lexer::read_ml_basic_string() {
    const char *sp = src.c_str() + pos;
    advance();
    advance();
    advance();
    if (peek() == '\n')
        advance();
    while (pos < src.length()) {
        char c = peek();
        if (c == '"') {
            if (peek_at(1) == '"' && peek_at(2) == '"') {
                const char *end = src.c_str() + pos;
                advance();
                advance();
                advance();
                return add_token(TokenType::TOK_STRING, sp,
                                 static_cast<size_t>(end - sp) + 3);
            } else
                advance();
        } else if (c == '\\') {
            advance();
            if (pos < src.length()) {
                char esc = advance();
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
                    for (int i = 0; i < 4 && pos < src.length(); i++) {
                        if (!is_ascii_hex_digit(advance())) {
                            set_error("invalid escape in multiline");
                            return false;
                        }
                    }
                    break;
                case 'U':
                    for (int i = 0; i < 8 && pos < src.length(); i++) {
                        if (!is_ascii_hex_digit(advance())) {
                            set_error("invalid escape in multiline");
                            return false;
                        }
                    }
                    break;
                default:
                    set_error("invalid escape in multiline string");
                    return false;
                }
            }
        } else
            advance();
    }
    set_error("unterminated multiline basic string");
    return false;
}

inline bool Lexer::read_ml_literal_string() {
    const char *sp = src.c_str() + pos;
    advance();
    advance();
    advance();
    if (peek() == '\n')
        advance();
    while (pos < src.length()) {
        char c = peek();
        if (c == '\'') {
            if (peek_at(1) == '\'' && peek_at(2) == '\'') {
                const char *end = src.c_str() + pos;
                advance();
                advance();
                advance();
                return add_token(TokenType::TOK_STRING, sp,
                                 static_cast<size_t>(end - sp) + 3);
            } else
                advance();
        } else
            advance();
    }
    set_error("unterminated multiline literal string");
    return false;
}

inline bool Lexer::read_bare_key() {
    const char *sp = src.c_str() + pos;
    while (pos < src.length() && is_bare_key_char(peek()))
        advance();
    size_t len = static_cast<size_t>((src.c_str() + pos) - sp);
    if (len == 0) {
        set_error("expected key");
        return false;
    }
    return add_token(TokenType::TOK_IDENT, sp, len);
}
inline bool Lexer::read_datetime() {
    const char *sp = src.c_str() + pos;
    for (int i = 0; i < 4 && pos < src.length() && is_ascii_digit(peek()); i++)
        advance();
    if (peek() == '-') {
        advance();
        for (int i = 0; i < 2 && pos < src.length() && is_ascii_digit(peek());
             i++)
            advance();
        if (peek() == '-') {
            advance();
            for (int i = 0;
                 i < 2 && pos < src.length() && is_ascii_digit(peek()); i++)
                advance();
        }
    }
    if (peek() == 'T' || peek() == 't' || peek() == ' ') {
        advance();
        for (int i = 0; i < 2 && pos < src.length() && is_ascii_digit(peek());
             i++)
            advance();
        if (peek() == ':') {
            advance();
            for (int i = 0;
                 i < 2 && pos < src.length() && is_ascii_digit(peek()); i++)
                advance();
            if (peek() == ':') {
                advance();
                for (int i = 0;
                     i < 2 && pos < src.length() && is_ascii_digit(peek()); i++)
                    advance();
                if (peek() == '.') {
                    advance();
                    while (pos < src.length() && is_ascii_digit(peek()))
                        advance();
                }
            }
        }
        if (peek() == 'Z' || peek() == 'z') {
            advance();
        } else if (peek() == '+' || peek() == '-') {
            advance();
            for (int i = 0;
                 i < 2 && pos < src.length() && is_ascii_digit(peek()); i++)
                advance();
            if (peek() == ':') {
                advance();
                for (int i = 0;
                     i < 2 && pos < src.length() && is_ascii_digit(peek()); i++)
                    advance();
            }
        }
    }
    return add_token(TokenType::TOK_DATETIME, sp,
                     static_cast<size_t>((src.c_str() + pos) - sp));
}
inline bool Lexer::read_number() {
    const char *sp = src.c_str() + pos;
    bool is_float = false;
    if (pos + 2 < src.length()) {
        const char *p = src.c_str() + pos;
        if ((p[0] == 'i' && p[1] == 'n' && p[2] == 'f') ||
            (p[0] == 'n' && p[1] == 'a' && p[2] == 'n')) {
            for (int i = 0; i < 3; i++)
                advance();
            if (peek() == '+' || peek() == '-')
                advance();
            return add_token(TokenType::TOK_FLOAT, sp,
                             static_cast<size_t>((src.c_str() + pos) - sp));
        }
    }
    if (peek() == '+' || peek() == '-')
        advance();
    bool is_hex = false, is_octal = false, is_binary = false;
    if (peek() == '0' && (peek_at(1) == 'x' || peek_at(1) == 'X')) {
        advance();
        advance();
        is_hex = true;
    } else if (peek() == '0' && (peek_at(1) == 'o' || peek_at(1) == 'O')) {
        advance();
        advance();
        is_octal = true;
    } else if (peek() == '0' && (peek_at(1) == 'b' || peek_at(1) == 'B')) {
        advance();
        advance();
        is_binary = true;
    }
    if (is_hex) {
        while (pos < src.length()) {
            char c = peek();
            if (is_ascii_hex_digit(c) || c == '_')
                advance();
            else
                break;
        }
    } else if (is_octal) {
        while (pos < src.length()) {
            char c = peek();
            if ((c >= '0' && c <= '7') || c == '_')
                advance();
            else
                break;
        }
    } else if (is_binary) {
        while (pos < src.length()) {
            char c = peek();
            if (c == '0' || c == '1' || c == '_')
                advance();
            else
                break;
        }
    } else {
        while (pos < src.length()) {
            char c = peek();
            if (is_ascii_digit(c) || c == '_')
                advance();
            else
                break;
        }
        if (peek() == '.' && pos + 1 < src.length() &&
            is_ascii_digit(peek_at(1))) {
            is_float = true;
            advance();
            while (pos < src.length()) {
                char c = peek();
                if (is_ascii_digit(c) || c == '_')
                    advance();
                else
                    break;
            }
        }
        if (peek() == 'e' || peek() == 'E') {
            is_float = true;
            advance();
            if (peek() == '+' || peek() == '-')
                advance();
            while (pos < src.length() && is_ascii_digit(peek()))
                advance();
        }
    }
    return add_token(is_float ? TokenType::TOK_FLOAT : TokenType::TOK_INTEGER,
                     sp, static_cast<size_t>((src.c_str() + pos) - sp));
}

inline bool Lexer::next_token() {
    if (!skip_whitespaces_and_comments())
        return false;
    if (pos >= src.length()) {
        return add_token(TokenType::TOK_EOF, src.c_str() + pos, 0);
    }
    char c = peek();
    if (c == '\n') {
        advance();
        return add_token(TokenType::TOK_NEWLINE, src.c_str() + pos - 1, 1);
    }
    if (c == '=') {
        advance();
        return add_token(TokenType::TOK_EQUAL, src.c_str() + pos - 1, 1);
    }
    if (c == '.') {
        advance();
        return add_token(TokenType::TOK_DOT, src.c_str() + pos - 1, 1);
    }
    if (c == ',') {
        advance();
        return add_token(TokenType::TOK_COMMA, src.c_str() + pos - 1, 1);
    }
    if (c == ':') {
        advance();
        return add_token(TokenType::TOK_COLON, src.c_str() + pos - 1, 1);
    }
    if (c == '{') {
        advance();
        return add_token(TokenType::TOK_LBRACE, src.c_str() + pos - 1, 1);
    }
    if (c == '}') {
        advance();
        return add_token(TokenType::TOK_RBRACE, src.c_str() + pos - 1, 1);
    }
    if (c == '[') {
        if (peek_at(1) == '[') {
            advance();
            advance();
            return add_token(TokenType::TOK_LLBRACKET, src.c_str() + pos - 2,
                             2);
        }
        advance();
        return add_token(TokenType::TOK_LBRACKET, src.c_str() + pos - 1, 1);
    }
    if (c == ']') {
        if (peek_at(1) == ']') {
            advance();
            advance();
            return add_token(TokenType::TOK_RRBRACKET, src.c_str() + pos - 2,
                             2);
        }
        advance();
        return add_token(TokenType::TOK_RBRACKET, src.c_str() + pos - 1, 1);
    }
    if (c == '"') {
        if (peek_at(1) == '"' && peek_at(2) == '"')
            return read_ml_basic_string();
        return read_string_basic();
    }
    if (c == '\'') {
        if (peek_at(1) == '\'' && peek_at(2) == '\'')
            return read_ml_literal_string();
        return read_string_literal();
    }
    if (is_ascii_digit(c)) {
        if (pos + 4 < src.length()) {
            const char *p = src.c_str() + pos;
            if (is_ascii_digit(p[0]) && is_ascii_digit(p[1]) &&
                is_ascii_digit(p[2]) && is_ascii_digit(p[3]) && p[4] == '-')
                return read_datetime();
        }
        return read_number();
    }
    if (c == '+' || c == '-') {
        if (pos + 1 < src.length() && is_ascii_digit(peek_at(1)))
            return read_number();
        if (pos + 3 < src.length()) {
            const char *p = src.c_str() + pos + 1;
            if ((p[0] == 'i' && p[1] == 'n' && p[2] == 'f') ||
                (p[0] == 'n' && p[1] == 'a' && p[2] == 'n'))
                return read_number();
        }
    }
    if (is_bare_key_char(c)) {
        if (match_keyword("true")) {
            for (int i = 0; i < 4; i++)
                advance();
            return add_token(TokenType::TOK_BOOL, src.c_str() + pos - 4, 4);
        }
        if (match_keyword("false")) {
            for (int i = 0; i < 5; i++)
                advance();
            return add_token(TokenType::TOK_BOOL, src.c_str() + pos - 5, 5);
        }
        return read_bare_key();
    }
    set_error("unexpected character");
    return false;
}

bool Lexer::Lexer_tokenize() {
    while (!has_error) {
        if (!next_token())
            return false;
        if (tokens.back()->type == TokenType::TOK_EOF)
            break;
    }
    return !has_error;
}
} // namespace Pico::TOML
