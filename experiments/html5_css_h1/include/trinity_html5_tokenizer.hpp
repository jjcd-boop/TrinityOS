#pragma once
#include <stddef.h>
#include <stdint.h>

namespace trinity::html5 {

enum class Kind : uint8_t { Text, StartTag, EndTag, Comment, Doctype, Eof };
struct Attribute {
    char name[48]{};
    char value[256]{};
    bool has_value{false};
};
struct Token {
    Kind kind{Kind::Eof};
    char name[48]{};
    char text[1024]{};
    size_t text_len{0};
    Attribute attrs[16]{};
    uint8_t attr_count{0};
    bool self_closing{false};
    bool overflow{false};
};
// Bounded, allocation-free subset of HTML Living Standard tokenization.
// UTF-8 source bytes are preserved, character references are emitted in UTF-8.
// Not yet a conforming complete HTML5 tokenizer or a DOM tree builder.
class Tokenizer {
public:
    Tokenizer(const char* source, size_t length) noexcept;
    Token next() noexcept;
private:
    enum class TextMode : uint8_t { Data, Rcdata, RawText };
    const char* data_;
    size_t size_;
    size_t pos_{0};
    TextMode mode_{TextMode::Data};
    char raw_element_[48]{};
    static bool whitespace(char c) noexcept;
    static bool alpha(char c) noexcept;
    static bool alnum(char c) noexcept;
    static char lower(char c) noexcept;
    bool matches(size_t at, const char* literal) const noexcept;
    bool appropriate_end(size_t at) const noexcept;
    void whitespace_skip() noexcept;
    void name_read(char* out, size_t capacity, bool& overflow) noexcept;
    static void append(char* out, size_t cap, size_t& size, bool& overflow, char ch) noexcept;
    static void append_utf8(char* out, size_t cap, size_t& size, bool& overflow, uint32_t cp) noexcept;
    bool char_ref(char* out, size_t cap, size_t& written, bool& overflow, bool attribute) noexcept;
    Token text() noexcept;
    Token tag(bool closing) noexcept;
    Token markup() noexcept;
    Token comment(size_t opener) noexcept;
};
} // namespace trinity::html5
