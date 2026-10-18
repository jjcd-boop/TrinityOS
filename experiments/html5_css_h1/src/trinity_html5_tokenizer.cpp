#include "trinity_html5_tokenizer.hpp"

namespace trinity::html5 {
Tokenizer::Tokenizer(const char* source, size_t length) noexcept
    : data_(source ? source : ""), size_(source ? length : 0) {}

bool Tokenizer::whitespace(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
}
bool Tokenizer::alpha(char c) noexcept { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
bool Tokenizer::alnum(char c) noexcept { return alpha(c) || (c >= '0' && c <= '9'); }
char Tokenizer::lower(char c) noexcept { return c >= 'A' && c <= 'Z' ? static_cast<char>(c + 32) : c; }
bool Tokenizer::matches(size_t at, const char* literal) const noexcept {
    if (!literal) return false;
    for (size_t i = 0; literal[i]; ++i)
        if (at + i >= size_ || lower(data_[at + i]) != lower(literal[i])) return false;
    return true;
}
void Tokenizer::whitespace_skip() noexcept {
    while (pos_ < size_ && whitespace(data_[pos_])) ++pos_;
}
void Tokenizer::name_read(char* out, size_t cap, bool& overflow) noexcept {
    size_t n = 0;
    while (pos_ < size_) {
        char c = data_[pos_];
        if (whitespace(c) || c == '>' || c == '/' || c == '=') break;
        append(out, cap, n, overflow, lower(c)); ++pos_;
    }
}
void Tokenizer::append(char* out, size_t cap, size_t& count, bool& overflow, char c) noexcept {
    if (count + 1 < cap) { out[count++] = c; out[count] = '\0'; }
    else overflow = true;
}
void Tokenizer::append_utf8(char* out, size_t cap, size_t& n, bool& overflow, uint32_t cp) noexcept {
    if (cp == 0 || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) cp = 0xfffd;
    char bytes[4]{}; size_t len = 0;
    if (cp < 0x80) { bytes[len++] = static_cast<char>(cp); }
    else if (cp < 0x800) {
        bytes[len++] = static_cast<char>(0xc0 | (cp >> 6));
        bytes[len++] = static_cast<char>(0x80 | (cp & 0x3f));
    } else if (cp < 0x10000) {
        bytes[len++] = static_cast<char>(0xe0 | (cp >> 12));
        bytes[len++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3f));
        bytes[len++] = static_cast<char>(0x80 | (cp & 0x3f));
    } else {
        bytes[len++] = static_cast<char>(0xf0 | (cp >> 18));
        bytes[len++] = static_cast<char>(0x80 | ((cp >> 12) & 0x3f));
        bytes[len++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3f));
        bytes[len++] = static_cast<char>(0x80 | (cp & 0x3f));
    }
    for (size_t i = 0; i < len; ++i) append(out, cap, n, overflow, bytes[i]);
}
bool Tokenizer::char_ref(char* out, size_t cap, size_t& n, bool& overflow, bool attribute) noexcept {
    (void)attribute;
    if (pos_ >= size_ || data_[pos_] != '&') return false;
    const size_t start = pos_;
    if (pos_ + 2 < size_ && data_[pos_+1] == '#') {
        size_t j = pos_ + 2; bool hex = false;
        if (j < size_ && (data_[j]=='x'||data_[j]=='X')) { hex = true; ++j; }
        const size_t digits = j;
        uint32_t value = 0; bool bad = false;
        while (j < size_) {
            const char c = data_[j];
            uint32_t d = 0;
            if (c >= '0' && c <= '9') d = static_cast<uint32_t>(c - '0');
            else if (hex && c >= 'a' && c <= 'f') d = static_cast<uint32_t>(c - 'a' + 10);
            else if (hex && c >= 'A' && c <= 'F') d = static_cast<uint32_t>(c - 'A' + 10);
            else break;
            if (value > (0x10ffffu - d) / (hex ? 16u : 10u)) bad = true;
            if (!bad) value = value * (hex ? 16u : 10u) + d;
            ++j;
        }
        if (j == digits) return false;
        if (j < size_ && data_[j] == ';') ++j;
        pos_ = j;
        if (bad) value = 0xfffd;
        // WHATWG numeric reference legacy windows-1252 replacements.
        static constexpr uint32_t windows1252[32] = {
            0x20ac,0x81,0x201a,0x192,0x201e,0x2026,0x2020,0x2021,
            0x2c6,0x2030,0x160,0x2039,0x152,0x8d,0x17d,0x8f,
            0x90,0x2018,0x2019,0x201c,0x201d,0x2022,0x2013,0x2014,
            0x2dc,0x2122,0x161,0x203a,0x153,0x9d,0x17e,0x178
        };
        if (value >= 0x80 && value <= 0x9f) value = windows1252[value - 0x80];
        append_utf8(out, cap, n, overflow, value);
        return true;
    }
    struct Named { const char* name; uint32_t code; };
    static constexpr Named refs[] = {
        {"amp;",'&'}, {"lt;",'<'}, {"gt;",'>'}, {"quot;",'"'},
        {"apos;",'\''}, {"nbsp;",0xa0}, {"copy;",0xa9},
        {"reg;",0xae}, {"mdash;",0x2014}, {"ndash;",0x2013},
        {"hellip;",0x2026}, {"euro;",0x20ac}
    };
    for (const auto& entry : refs) {
        size_t j = start + 1;
        const char* s = entry.name;
        size_t k = 0;
        while (s[k] && j < size_ && s[k] == data_[j]) { ++j; ++k; }
        if (!s[k]) {
            pos_ = j;
            append_utf8(out, cap, n, overflow, entry.code);
            return true;
        }
    }
    return false;
}
bool Tokenizer::appropriate_end(size_t at) const noexcept {
    if (at + 3 >= size_ || data_[at] != '<' || data_[at+1] != '/' || !raw_element_[0]) return false;
    size_t k = 0, p = at + 2;
    while (raw_element_[k] && p < size_ && lower(data_[p]) == raw_element_[k]) { ++p; ++k; }
    if (raw_element_[k] || p >= size_) return false;
    return whitespace(data_[p]) || data_[p] == '>' || data_[p] == '/';
}
Token Tokenizer::text() noexcept {
    Token t{}; t.kind = Kind::Text;
    while (pos_ < size_) {
        if (t.text_len + 5 >= sizeof(t.text)) break;
        char c = data_[pos_];
        if (c == '<' && (mode_ == TextMode::Data || appropriate_end(pos_))) break;
        if (c == '&' && mode_ != TextMode::RawText &&
            char_ref(t.text, sizeof(t.text), t.text_len, t.overflow, false)) continue;
        append(t.text, sizeof(t.text), t.text_len, t.overflow, c); ++pos_;
    }
    return t;
}
Token Tokenizer::comment(size_t opener) noexcept {
    Token t{}; t.kind = Kind::Comment; pos_ += opener;
    while (pos_ < size_) {
        if (pos_+2 < size_ && data_[pos_]=='-' && data_[pos_+1]=='-' && data_[pos_+2]=='>') {
            pos_ += 3; return t;
        }
        if (opener == 2 && data_[pos_] == '>') { ++pos_; return t; }
        append(t.text, sizeof(t.text), t.text_len, t.overflow, data_[pos_++]);
    }
    return t;
}
Token Tokenizer::markup() noexcept {
    if (matches(pos_, "<!--")) return comment(4);
    if (matches(pos_, "<!doctype") &&
        (pos_+9==size_ || whitespace(data_[pos_+9]) || data_[pos_+9]=='>')) {
        Token t{}; t.kind = Kind::Doctype; pos_ += 9;
        whitespace_skip();
        name_read(t.name, sizeof(t.name), t.overflow);
        char quote = 0;
        while (pos_ < size_) {
            const char ch=data_[pos_++];
            if (quote) { if (ch == quote) quote = 0; }
            else if (ch=='\''||ch=='"') quote=ch;
            else if (ch=='>') break;
        }
        return t;
    }
    return comment(2);
}
Token Tokenizer::tag(bool closing) noexcept {
    Token t{}; t.kind = closing ? Kind::EndTag : Kind::StartTag;
    pos_ += closing ? 2 : 1;
    name_read(t.name, sizeof(t.name), t.overflow);
    if (!t.name[0]) { t.overflow = true; }
    while (pos_ < size_) {
        whitespace_skip();
        if (pos_ >= size_) break;
        const char c = data_[pos_];
        if (c == '>') { ++pos_; break; }
        if (c == '/' && pos_+1 < size_ && data_[pos_+1] == '>') {
            pos_ += 2; t.self_closing = !closing; break;
        }
        if (closing) { ++pos_; continue; }
        if (c == '/') { ++pos_; continue; }
        Attribute candidate{}; bool candidate_overflow = false;
        const size_t before = pos_;
        name_read(candidate.name, sizeof(candidate.name), candidate_overflow);
        if (pos_ == before) { ++pos_; continue; }
        whitespace_skip();
        if (pos_ < size_ && data_[pos_] == '=') {
            ++pos_; whitespace_skip(); candidate.has_value = true;
            char quote = 0;
            if (pos_ < size_ && (data_[pos_]=='"' || data_[pos_]=='\'')) quote = data_[pos_++];
            size_t count = 0;
            while (pos_ < size_) {
                char ch = data_[pos_];
                if (quote && ch == quote) { ++pos_; break; }
                if (!quote && (whitespace(ch) || ch == '>')) break;
                if (ch == '&' && char_ref(candidate.value, sizeof(candidate.value), count, candidate_overflow, true)) continue;
                append(candidate.value, sizeof(candidate.value), count, candidate_overflow, ch); ++pos_;
            }
        }
        bool duplicate = false;
        for (uint8_t i = 0; i < t.attr_count; ++i) {
            size_t j = 0;
            while (candidate.name[j] && t.attrs[i].name[j] == candidate.name[j]) ++j;
            if (!candidate.name[j] && !t.attrs[i].name[j]) { duplicate = true; break; }
        }
        if (!duplicate) {
            if (t.attr_count < 16) t.attrs[t.attr_count++] = candidate;
            else t.overflow = true;
        }
        if (candidate_overflow) t.overflow = true;
    }
    if (closing) {
        if (mode_ != TextMode::Data) {
            bool same=true;size_t i=0;
            for (; raw_element_[i] && t.name[i]; ++i)
                if (raw_element_[i]!=t.name[i]) { same=false; break; }
            if (same && !raw_element_[i] && !t.name[i]) {
                mode_=TextMode::Data; raw_element_[0]=0;
            }
        }
    } else if (!t.self_closing) {
        bool is_raw = false;
        const char* names[]={"script","style","xmp","iframe","noembed","noframes"};
        for (const char* name : names) {
            size_t j=0; while(name[j] && t.name[j]==name[j]) ++j;
            if (!name[j] && !t.name[j]) { is_raw=true; break; }
        }
        bool rcdata = false;
        for (const char* name : {"title", "textarea"}) {
            size_t j=0; while (name[j] && t.name[j]==name[j]) ++j;
            if (!name[j] && !t.name[j]) { rcdata=true; break; }
        }
        if (is_raw || rcdata) {
            mode_=is_raw?TextMode::RawText:TextMode::Rcdata;
            size_t i=0;for(;i+1<sizeof(raw_element_) && t.name[i];++i)raw_element_[i]=t.name[i];
            raw_element_[i]=0;
        }
    }
    return t;
}
Token Tokenizer::next() noexcept {
    if (pos_ >= size_) return Token{};
    if (mode_ != TextMode::Data) {
        if (appropriate_end(pos_)) return tag(true);
        return text();
    }
    if (data_[pos_] == '<') {
        if (pos_+1 < size_) {
            if (data_[pos_+1]=='!') return markup();
            if (data_[pos_+1]=='?' ) return comment(2);
            if (data_[pos_+1]=='/' && pos_+2 < size_ && alpha(data_[pos_+2])) return tag(true);
            if (alpha(data_[pos_+1])) return tag(false);
        }
        Token t{}; t.kind=Kind::Text;
        t.text[0]='<'; t.text_len=1; ++pos_; return t;
    }
    return text();
}
} // namespace trinity::html5
