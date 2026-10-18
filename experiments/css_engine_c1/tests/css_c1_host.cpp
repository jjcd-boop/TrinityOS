#include "trinity_css_c1.hpp"
#include <cstdio>
#include <cstring>
#include <string>
using namespace trinity::css_c1;
static Stylesheet sheet;
static int errors=0;
static void check(bool yes,const char* what){
    if(!yes){++errors;std::printf("FAIL %s\n",what);}
}
static bool eq(const char* a,const char* b){return std::strcmp(a,b)==0;}
static void check_token(const char* src,TokenKind kind,const char* value,
                        const char* unit=""){
    Tokenizer t(src,std::strlen(src));
    Token x=t.next();
    check(x.kind==kind && eq(x.value,value) && eq(x.unit,unit),
          "CSS token type and value");
    check(!x.truncated,"token not truncated");
}
int main(){
    check_token("  \n ",TokenKind::Whitespace,"");
    check_token("/* comments */some-name",TokenKind::Ident,"some-name");
    check_token("@media",TokenKind::AtKeyword,"media");
    check_token("#abc123",TokenKind::Hash,"abc123");
    check_token("\"hello \\41\"",TokenKind::String,"hello A");
    check_token("5px",TokenKind::Dimension,"5","px");
    check_token("12.5%",TokenKind::Percentage,"12.5");
    check_token("-1.2e3rem",TokenKind::Dimension,"-1.2e3","rem");
    check_token("url(images/foo.png)",TokenKind::Url,"images/foo.png");
    check_token("url(a b)",TokenKind::BadUrl,"a");
    check_token("calc(10px)",TokenKind::Function,"calc");
    check_token("--brand",TokenKind::Ident,"--brand");
    check_token("c\\6flor",TokenKind::Ident,"color");
    {
        const char* css="/* hello */ h1, .card > p { color: red; margin: 12px; --gap: calc(100% - 5px); }";
        check(sheet.parse(css,std::strlen(css)),"parse stylesheet");
        check(sheet.count()==1,"one rule");
        auto* r=sheet.get(0);
        check(r&&eq(r->selector,"h1, .card > p"),"selector retained");
        check(r&&r->declaration_count==3,"three declarations");
        check(eq(r->declarations[0].property,"color")&&
              eq(r->declarations[0].value,"red"),"color declaration");
        check(eq(r->declarations[1].value,"12px"),"dimension value");
        check(eq(r->declarations[2].value,"calc(100% - 5px)"),"nested calc kept");
    }
    {
        const char* css="a[href='x;y'] { content: 'a;b'; background:url(data:image/svg+xml;a=b); color: RED ! important; }";
        check(sheet.parse(css,std::strlen(css)),"quoted semicolons and url");
        check(sheet.count()==1&&sheet.get(0)->declaration_count==3,
              "semicolons within string and url not declaration separators");
        const Rule* r=sheet.get(0);
        check(eq(r->declarations[0].value,"'a;b'"),"quoted semicolon");
        check(eq(r->declarations[1].value,"url(data:image/svg+xml;a=b)"),
              "url semicolon kept");
        check(r->declarations[2].important &&
              eq(r->declarations[2].value,"RED"),"important priority");
    }
    {
        const char* css="@charset \"utf-8\"; @media screen { .hide { color: blue; } } .show { display:block; }";
        check(sheet.parse(css,std::strlen(css)),"skip at rules");
        check(sheet.count()==1&&eq(sheet.get(0)->selector,".show"),
              "skipped nested at-rule never becomes a selector");
    }
    {
        const char* css="a{color:red; invalid; width:2px; color:blue}";
        check(sheet.parse(css,std::strlen(css)),"invalid declaration recovery");
        check(sheet.get(0)->declaration_count==3,"bad declaration skipped");
        check(eq(sheet.get(0)->declarations[2].value,"blue"),"duplicate property preserved");
    }
    {
        const char* css="a { background: linear-gradient(red, rgb(0,0,0)); }";
        check(sheet.parse(css,std::strlen(css)),"nested function value");
        check(eq(sheet.get(0)->declarations[0].value,
                 "linear-gradient(red, rgb(0,0,0))"),"nested paren value");
    }
    {
        std::string css;
        for(int i=0;i<70;++i)css+=".a{color:red;}";
        check(!sheet.parse(css.data(),css.size())&&sheet.error()==ParseError::RuleLimit,
              "fail closed on excessive rules");
    }
    {
        std::string css="a{color:"+std::string(600,'x')+";}";
        check(!sheet.parse(css.data(),css.size())&&sheet.error()==ParseError::TokenOverflow,
              "fail closed when a token exceeds its bounded buffer");
    }
    {
        std::string css="a{";
        for(int i=0;i<30;++i)css+="x:y;";
        css+="}";
        check(!sheet.parse(css.data(),css.size())&&
              sheet.error()==ParseError::DeclarationLimit,
              "fail closed on declaration limit");
    }
    {
        const char* css="a{color:red;}";
        check(sheet.parse(css,std::strlen(css))&&sheet.count()==1,
              "reusable parser after resource-limit failures");
    }
    if(errors){std::printf("FAIL CSS C1 %d checks\n",errors);return 1;}
    std::puts("PASS CSS C1 tokenizer + bounded stylesheet/declaration parser");
    return 0;
}
