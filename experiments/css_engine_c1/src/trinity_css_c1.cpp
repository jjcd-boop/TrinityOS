#include "trinity_css_c1.hpp"

namespace trinity::css_c1 {
Tokenizer::Tokenizer(const char* bytes,size_t length) noexcept
    : src_(bytes?bytes:""), len_(bytes?length:0) {}
bool Tokenizer::space(unsigned char c) const noexcept {
    return c==' '||c=='\n'||c=='\t'||c=='\r'||c=='\f';
}
bool Tokenizer::alpha(unsigned char c) noexcept {return (c>='A'&&c<='Z')||(c>='a'&&c<='z');}
bool Tokenizer::digit(unsigned char c) noexcept {return c>='0'&&c<='9';}
bool Tokenizer::ident_start(unsigned char c) noexcept {
    return alpha(c)||c=='_'||c>=0x80;
}
bool Tokenizer::ident_char(unsigned char c) noexcept {
    return ident_start(c)||digit(c)||c=='-';
}
bool Tokenizer::valid_escape(size_t at) const noexcept {
    return at+1<len_ && src_[at]=='\\' && src_[at+1]!='\n' &&
           src_[at+1]!='\r' && src_[at+1]!='\f';
}
bool Tokenizer::starts_ident(size_t at) const noexcept {
    if(at>=len_)return false;
    const unsigned char c=static_cast<unsigned char>(src_[at]);
    if(c=='-') {
        if(at+1>=len_)return false;
        const auto second=static_cast<unsigned char>(src_[at+1]);
        return ident_start(second)||second=='-'||valid_escape(at+1);
    }
    return ident_start(c)||valid_escape(at);
}
bool Tokenizer::starts_number(size_t at) const noexcept {
    if(at>=len_)return false;
    size_t p=at;
    if(src_[p]=='+'||src_[p]=='-')++p;
    if(p>=len_)return false;
    if(digit(static_cast<unsigned char>(src_[p])))return true;
    return src_[p]=='.' && p+1<len_ &&
        digit(static_cast<unsigned char>(src_[p+1]));
}
void Tokenizer::push(char* dest,size_t cap,size_t& used,char c,bool& overflow) noexcept {
    if(used+1<cap){dest[used++]=c;dest[used]=0;}
    else overflow=true;
}
void Tokenizer::append_utf8(char* dest,size_t cap,size_t& used,uint32_t cp,bool& overflow) noexcept {
    if(cp==0||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))cp=0xfffd;
    if(cp<128){push(dest,cap,used,static_cast<char>(cp),overflow);return;}
    if(cp<2048){
        push(dest,cap,used,static_cast<char>(0xc0|(cp>>6)),overflow);
        push(dest,cap,used,static_cast<char>(0x80|(cp&63)),overflow);return;
    }
    if(cp<65536){
        push(dest,cap,used,static_cast<char>(0xe0|(cp>>12)),overflow);
        push(dest,cap,used,static_cast<char>(0x80|((cp>>6)&63)),overflow);
        push(dest,cap,used,static_cast<char>(0x80|(cp&63)),overflow);return;
    }
    push(dest,cap,used,static_cast<char>(0xf0|(cp>>18)),overflow);
    push(dest,cap,used,static_cast<char>(0x80|((cp>>12)&63)),overflow);
    push(dest,cap,used,static_cast<char>(0x80|((cp>>6)&63)),overflow);
    push(dest,cap,used,static_cast<char>(0x80|(cp&63)),overflow);
}
void Tokenizer::consume_escape(char* dst,size_t cap,size_t& used,bool& overflow) noexcept {
    // Caller established a valid backslash escape.
    ++pos_;
    uint32_t cp=0;size_t digits=0;
    while(pos_<len_ && digits<6){
        const char c=src_[pos_];uint32_t value=0;
        if(c>='0'&&c<='9')value=static_cast<uint32_t>(c-'0');
        else if(c>='a'&&c<='f')value=static_cast<uint32_t>(c-'a'+10);
        else if(c>='A'&&c<='F')value=static_cast<uint32_t>(c-'A'+10);
        else break;
        cp=cp*16+value;++digits;++pos_;
    }
    if(digits){
        if(pos_<len_ && space(static_cast<unsigned char>(src_[pos_])))++pos_;
        append_utf8(dst,cap,used,cp,overflow);
    }else if(pos_<len_){
        push(dst,cap,used,src_[pos_++],overflow);
    }
}
void Tokenizer::consume_name(char* dst,size_t cap,size_t& used,bool& overflow) noexcept {
    while(pos_<len_){
        const unsigned char c=static_cast<unsigned char>(src_[pos_]);
        if(ident_char(c)){push(dst,cap,used,static_cast<char>(c),overflow);++pos_;}
        else if(valid_escape(pos_))consume_escape(dst,cap,used,overflow);
        else break;
    }
}
Token Tokenizer::make(TokenKind kind,size_t start) const noexcept {
    Token token{};token.kind=kind;token.begin=start;token.end=pos_;return token;
}
Token Tokenizer::consume_string(char quote,size_t begin) noexcept {
    Token out=make(TokenKind::String,begin);
    size_t used=0;
    while(pos_<len_) {
        const char c=src_[pos_];
        if(c==quote){++pos_;out.end=pos_;return out;}
        if(c=='\n'||c=='\r'||c=='\f'){out.kind=TokenKind::BadString;out.end=pos_;return out;}
        if(c=='\\'){
            if(pos_+1>=len_){++pos_;break;}
            if(src_[pos_+1]=='\n'||src_[pos_+1]=='\f'){
                pos_+=2;continue;
            }
            if(src_[pos_+1]=='\r'){
                pos_+=2;if(pos_<len_ && src_[pos_]=='\n')++pos_;
                continue;
            }
            if(valid_escape(pos_)){consume_escape(out.value,sizeof(out.value),used,out.truncated);continue;}
        }
        push(out.value,sizeof(out.value),used,c,out.truncated);++pos_;
    }
    out.end=pos_;return out;
}
Token Tokenizer::consume_url(size_t begin) noexcept {
    Token out=make(TokenKind::Url,begin);
    size_t used=0;
    while(pos_<len_ && space(static_cast<unsigned char>(src_[pos_])))++pos_;
    while(pos_<len_) {
        const char c=src_[pos_++];
        if(c==')'){out.end=pos_;return out;}
        if(space(static_cast<unsigned char>(c))){
            while(pos_<len_ && space(static_cast<unsigned char>(src_[pos_])))++pos_;
            if(pos_<len_ && src_[pos_]==')'){++pos_;out.end=pos_;return out;}
            out.kind=TokenKind::BadUrl;
        }else if(c=='"'||c=='\''||c=='('||static_cast<unsigned char>(c)<0x20){
            out.kind=TokenKind::BadUrl;
        }else if(c=='\\') {
            --pos_;
            if(valid_escape(pos_))consume_escape(out.value,sizeof(out.value),used,out.truncated);
            else{++pos_;out.kind=TokenKind::BadUrl;}
        }else push(out.value,sizeof(out.value),used,c,out.truncated);
        if(out.kind==TokenKind::BadUrl){
            // Consume the rest of the invalid URL, respecting escaped right parentheses.
            while(pos_<len_){
                if(src_[pos_]==')'){++pos_;break;}
                if(valid_escape(pos_)){pos_+=2;continue;}
                ++pos_;
            }
            out.end=pos_;return out;
        }
    }
    out.end=pos_;return out;
}
Token Tokenizer::consume_number() noexcept {
    const size_t start=pos_;
    Token out=make(TokenKind::Number,start);
    size_t used=0;
    if(pos_<len_ && (src_[pos_]=='+'||src_[pos_]=='-'))
        push(out.value,sizeof(out.value),used,src_[pos_++],out.truncated);
    while(pos_<len_ && digit(static_cast<unsigned char>(src_[pos_])))
        push(out.value,sizeof(out.value),used,src_[pos_++],out.truncated);
    if(pos_+1<len_ && src_[pos_]=='.' &&
       digit(static_cast<unsigned char>(src_[pos_+1]))){
        push(out.value,sizeof(out.value),used,src_[pos_++],out.truncated);
        while(pos_<len_ && digit(static_cast<unsigned char>(src_[pos_])))
            push(out.value,sizeof(out.value),used,src_[pos_++],out.truncated);
    }
    if(pos_<len_ && (src_[pos_]=='e'||src_[pos_]=='E')){
        size_t p=pos_+1;
        if(p<len_ && (src_[p]=='+'||src_[p]=='-'))++p;
        if(p<len_ && digit(static_cast<unsigned char>(src_[p]))){
            while(pos_<p+1)push(out.value,sizeof(out.value),used,src_[pos_++],out.truncated);
            while(pos_<len_ && digit(static_cast<unsigned char>(src_[pos_])))
                push(out.value,sizeof(out.value),used,src_[pos_++],out.truncated);
        }
    }
    if(pos_<len_ && src_[pos_]=='%'){++pos_;out.kind=TokenKind::Percentage;}
    else if(starts_ident(pos_)){
        size_t unit_size=0;out.kind=TokenKind::Dimension;
        consume_name(out.unit,sizeof(out.unit),unit_size,out.truncated);
    }
    out.end=pos_;return out;
}
Token Tokenizer::next() noexcept {
    for(;;){
        if(pos_>=len_)return make(TokenKind::End,pos_);
        if(pos_+1<len_ && src_[pos_]=='/'&&src_[pos_+1]=='*'){
            pos_+=2;
            while(pos_<len_){
                if(pos_+1<len_&&src_[pos_]=='*'&&src_[pos_+1]=='/'){
                    pos_+=2;break;
                }
                ++pos_;
            }
            continue;
        }
        break;
    }
    const size_t start=pos_;
    const char c=src_[pos_];
    if(space(static_cast<unsigned char>(c))){
        do{++pos_;}while(pos_<len_&&space(static_cast<unsigned char>(src_[pos_])));
        return make(TokenKind::Whitespace,start);
    }
    if(c=='"'||c=='\''){++pos_;return consume_string(c,start);}
    if(starts_number(pos_))return consume_number();
    if(c=='@'&&starts_ident(pos_+1)){
        ++pos_;Token t=make(TokenKind::AtKeyword,start);size_t used=0;
        consume_name(t.value,sizeof(t.value),used,t.truncated);t.end=pos_;return t;
    }
    if(c=='#' && (pos_+1<len_ &&
       (ident_char(static_cast<unsigned char>(src_[pos_+1]))||valid_escape(pos_+1)))){
        ++pos_;Token t=make(TokenKind::Hash,start);size_t used=0;
        consume_name(t.value,sizeof(t.value),used,t.truncated);t.end=pos_;return t;
    }
    if(starts_ident(pos_)){
        Token t=make(TokenKind::Ident,start);size_t used=0;
        consume_name(t.value,sizeof(t.value),used,t.truncated);
        if(pos_<len_&&src_[pos_]=='('){
            ++pos_;t.kind=TokenKind::Function;
            if(t.value[0]=='u'&&t.value[1]=='r'&&t.value[2]=='l'&&!t.value[3]){
                size_t p=pos_;
                while(p<len_&&space(static_cast<unsigned char>(src_[p])))++p;
                if(p<len_&&src_[p]!='"'&&src_[p]!='\'')return consume_url(start);
            }
        }
        t.end=pos_;return t;
    }
    ++pos_;
    TokenKind kind=TokenKind::Delim;
    switch(c){
        case ':':kind=TokenKind::Colon;break;
        case ';':kind=TokenKind::Semicolon;break;
        case ',':kind=TokenKind::Comma;break;
        case '{':kind=TokenKind::LeftBrace;break;
        case '}':kind=TokenKind::RightBrace;break;
        case '(':kind=TokenKind::LeftParen;break;
        case ')':kind=TokenKind::RightParen;break;
        case '[':kind=TokenKind::LeftBracket;break;
        case ']':kind=TokenKind::RightBracket;break;
        default:break;
    }
    Token t=make(kind,start);t.delim=c;
    return t;
}
bool Stylesheet::append(char* out,size_t cap,size_t& used,const char* s,size_t n) noexcept {
    if(n>=cap || used>cap-1-n)return false;
    for(size_t i=0;i<n;++i)out[used+i]=s[i];
    used+=n;out[used]=0;return true;
}
void Stylesheet::trim(char* text) noexcept {
    size_t count=0;
    while(text[count])++count;
    size_t start=0;
    while(start<count && (text[start]==' '||text[start]=='\n'||text[start]=='\t'||text[start]=='\r'))++start;
    while(count>start && (text[count-1]==' '||text[count-1]=='\n'||text[count-1]=='\t'||text[count-1]=='\r'))--count;
    const size_t n=count-start;
    for(size_t i=0;i<n;++i)text[i]=text[start+i];
    text[n]=0;
}
bool Stylesheet::read_property(const char* source,size_t n,char* out,size_t cap) noexcept {
    if(!n||n>=cap)return false;
    for(size_t i=0;i<n;++i) {
        const unsigned char c=static_cast<unsigned char>(source[i]);
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
             (c>='0'&&c<='9')||c=='-'||c=='_'))return false;
        out[i]=(c>='A'&&c<='Z')?static_cast<char>(c+32):static_cast<char>(c);
    }
    out[n]=0;
    return true;
}
bool Stylesheet::important(char* text) noexcept {
    size_t n=0;while(text[n])++n;
    if(n<10)return false;
    // Remove final !important, with optional whitespace after !.
    size_t j=n;while(j && (text[j-1]==' '||text[j-1]=='\t'))--j;
    const char* marker="important";const size_t m=9;
    if(j<m)return false;
    size_t p=j-m;
    for(size_t i=0;i<m;++i){
        char c=text[p+i];if(c>='A'&&c<='Z')c=static_cast<char>(c+32);
        if(c!=marker[i])return false;
    }
    while(p && (text[p-1]==' '||text[p-1]=='\t'))--p;
    if(!p || text[p-1]!='!')return false;
    text[p-1]=0;trim(text);return true;
}
bool Stylesheet::parse_declarations(const char* s,size_t begin,size_t end,Rule& rule) noexcept {
    size_t pos=begin;
    while(pos<end) {
        // Skip whitespace/semicolons/comments between declarations.
        while(pos<end && (s[pos]==' '||s[pos]=='\t'||s[pos]=='\n'||
                           s[pos]=='\r'||s[pos]==';'))++pos;
        if(pos+1<end && s[pos]=='/'&&s[pos+1]=='*'){
            pos+=2;while(pos+1<end && !(s[pos]=='*'&&s[pos+1]=='/'))++pos;
            pos=(pos+1<end)?pos+2:end;continue;
        }
        if(pos>=end)break;
        size_t prop_start=pos;
        while(pos<end && s[pos]!=':'&&s[pos]!=';'&&s[pos]!='}')++pos;
        if(pos>=end||s[pos]!=':'){
            while(pos<end&&s[pos]!=';')++pos;
            if(pos<end)++pos;continue;
        }
        size_t prop_end=pos;
        while(prop_end>prop_start && (s[prop_end-1]==' '||s[prop_end-1]=='\t'))--prop_end;
        while(prop_start<prop_end && (s[prop_start]==' '||s[prop_start]=='\t'))++prop_start;
        ++pos;
        Declaration decl{};
        if(!read_property(s+prop_start,prop_end-prop_start,decl.property,sizeof(decl.property))){
            // Unknown or malformed property syntax is skipped, not promoted to
            // a valid declaration.
            while(pos<end && s[pos]!=';')++pos;
            if(pos<end)++pos;continue;
        }
        size_t count=0;
        char quote=0;int parens=0,brackets=0,braces=0;
        while(pos<end){
            const char c=s[pos];
            if(!quote && pos+1<end && c=='/' && s[pos+1]=='*'){
                pos+=2;
                while(pos+1<end && !(s[pos]=='*'&&s[pos+1]=='/'))++pos;
                pos=pos+1<end?pos+2:end;continue;
            }
            if(c=='\\' && pos+1<end){
                if(!append(decl.value,sizeof(decl.value),count,s+pos,2)){
                    error_=ParseError::ValueLimit;return false;
                }
                pos+=2;continue;
            }
            if(quote) {
                if(c==quote)quote=0;
            } else {
                if(c=='"'||c=='\'')quote=c;
                else if(c=='(')++parens;
                else if(c==')'&&parens>0)--parens;
                else if(c=='[')++brackets;
                else if(c==']'&&brackets>0)--brackets;
                else if(c=='{')++braces;
                else if(c=='}'&&braces>0)--braces;
                else if(c==';'&&parens==0&&brackets==0&&braces==0)break;
            }
            if(!append(decl.value,sizeof(decl.value),count,s+pos,1)){
                error_=ParseError::ValueLimit;return false;
            }
            ++pos;
        }
        trim(decl.value);decl.important=important(decl.value);
        if(rule.declaration_count>=24){error_=ParseError::DeclarationLimit;return false;}
        rule.declarations[rule.declaration_count++]=decl;
        if(pos<end&&s[pos]==';')++pos;
    }
    return true;
}
bool Stylesheet::parse(const char* bytes,size_t len) noexcept {
    count_=0;error_=ParseError::None;
    if(!bytes&&len){error_=ParseError::InvalidRule;return false;}
    if(!bytes){bytes="";len=0;}
    Tokenizer tokens(bytes,len);
    for(size_t token_limit=0;token_limit<32768;++token_limit){
        Token first=tokens.next();
        if(first.kind==TokenKind::End)return true;
        if(first.truncated){error_=ParseError::TokenOverflow;return false;}
        if(first.kind==TokenKind::Whitespace||first.kind==TokenKind::Semicolon)continue;
        if(first.kind==TokenKind::AtKeyword){
            // C1 deliberately skips @rules; C2 will parse @media and @supports.
            int nested=0;
            while(true){
                Token t=tokens.next();
                if(t.kind==TokenKind::End) return true;
                if(t.truncated){error_=ParseError::TokenOverflow;return false;}
                if(t.kind==TokenKind::LeftBrace)++nested;
                else if(t.kind==TokenKind::RightBrace && nested>0) {
                    if(--nested==0)break;
                }
                else if(t.kind==TokenKind::Semicolon && nested==0)break;
            }
            continue;
        }
        size_t begin=first.begin,end=first.end;
        int brackets=0,parens=0;
        Token token=first;
        bool found=false;
        for(size_t scan=0;scan<8192;++scan){
            if(token.kind==TokenKind::LeftBracket)++brackets;
            else if(token.kind==TokenKind::RightBracket&&brackets>0)--brackets;
            else if(token.kind==TokenKind::LeftParen||token.kind==TokenKind::Function)++parens;
            else if(token.kind==TokenKind::RightParen&&parens>0)--parens;
            else if(token.kind==TokenKind::LeftBrace && parens==0&&brackets==0){
                found=true;end=token.begin;break;
            }
            if(token.kind==TokenKind::End)break;
            if(token.truncated){error_=ParseError::TokenOverflow;return false;}
            token=tokens.next();
        }
        if(!found){error_=ParseError::InvalidRule;return false;}
        if(count_>=kMaxRules){error_=ParseError::RuleLimit;return false;}
        Rule& rule=rules_[count_];rule=Rule{};
        if(end<=begin){error_=ParseError::InvalidRule;return false;}
        size_t n=0;
        if(!append(rule.selector,sizeof(rule.selector),n,bytes+begin,end-begin)){
            error_=ParseError::SelectorLimit;return false;
        }
        trim(rule.selector);
        if(!rule.selector[0]){error_=ParseError::InvalidRule;return false;}
        const size_t block_begin=tokens.offset();
        size_t block_end=block_begin;
        int depth=1;
        for(size_t scan=0;scan<32768;++scan){
            Token t=tokens.next();
            if(t.truncated){error_=ParseError::TokenOverflow;return false;}
            if(t.kind==TokenKind::End){error_=ParseError::InvalidRule;return false;}
            if(t.kind==TokenKind::LeftBrace)++depth;
            else if(t.kind==TokenKind::RightBrace){
                if(--depth==0){block_end=t.begin;break;}
            }
        }
        if(depth){error_=ParseError::InvalidRule;return false;}
        if(!parse_declarations(bytes,block_begin,block_end,rule))return false;
        ++count_;
    }
    error_=ParseError::TokenOverflow;return false;
}
} // namespace trinity::css_c1
