#pragma once
#include <stddef.h>
#include <stdint.h>

namespace trinity::css_c1 {
// Standalone, allocation-free CSS Syntax Level 3 foundation.
// Not a complete W3C CSS parser, cascade, selector matcher or computed style.
enum class TokenKind : uint8_t {
    End, Whitespace, Ident, Function, AtKeyword, Hash, String, BadString,
    Number, Percentage, Dimension, Colon, Semicolon, Comma,
    LeftBrace, RightBrace, LeftParen, RightParen, LeftBracket, RightBracket,
    Delim, BadUrl, Url
};
struct Token {
    TokenKind kind{TokenKind::End};
    char value[256]{};
    char unit[48]{};
    size_t begin{0}, end{0};
    char delim{0};
    bool truncated{false};
};
class Tokenizer {
public:
    Tokenizer(const char* bytes,size_t length) noexcept;
    Token next() noexcept;
    size_t offset() const noexcept { return pos_; }
private:
    const char* src_;
    size_t len_,pos_{0};
    bool space(unsigned char c) const noexcept;
    static bool alpha(unsigned char c) noexcept;
    static bool digit(unsigned char c) noexcept;
    static bool ident_start(unsigned char c) noexcept;
    static bool ident_char(unsigned char c) noexcept;
    bool starts_ident(size_t at) const noexcept;
    bool starts_number(size_t at) const noexcept;
    bool valid_escape(size_t at) const noexcept;
    static void push(char* dest,size_t capacity,size_t& used,char c,bool& overflow) noexcept;
    static void append_utf8(char* dest,size_t capacity,size_t& used,uint32_t code,bool& overflow) noexcept;
    void consume_escape(char* dest,size_t capacity,size_t& used,bool& overflow) noexcept;
    void consume_name(char* dest,size_t capacity,size_t& used,bool& overflow) noexcept;
    Token consume_number() noexcept;
    Token consume_string(char quote,size_t begin) noexcept;
    Token consume_url(size_t begin) noexcept;
    Token make(TokenKind kind,size_t begin) const noexcept;
};
enum class ParseError : uint8_t {
    None, RuleLimit, DeclarationLimit, SelectorLimit, PropertyLimit,
    ValueLimit, InvalidRule, TokenOverflow
};
struct Declaration {
    char property[64]{};
    char value[512]{};
    bool important{false};
};
struct Rule {
    char selector[256]{};
    Declaration declarations[24]{};
    uint8_t declaration_count{0};
};
// Bounded collection of simple CSS qualified rules and declarations.
// @-rules are skipped atomically, never interpreted as normal selectors.
// Selectors/values are kept as source text for C2 parsing and cascade.
class Stylesheet {
public:
    static constexpr size_t kMaxRules=64;
    bool parse(const char* bytes,size_t length) noexcept;
    size_t count() const noexcept { return count_; }
    const Rule* get(size_t i) const noexcept {return i<count_?&rules_[i]:nullptr;}
    ParseError error() const noexcept {return error_;}
private:
    Rule rules_[kMaxRules]{};
    size_t count_{0};
    ParseError error_{ParseError::None};
    static bool append(char* out,size_t cap,size_t& used,const char* src,size_t n) noexcept;
    static void trim(char* text) noexcept;
    static bool important(char* text) noexcept;
    static bool read_property(const char* s,size_t size,char* out,size_t cap) noexcept;
    bool parse_declarations(const char* s,size_t begin,size_t end,Rule& rule) noexcept;
};
}
