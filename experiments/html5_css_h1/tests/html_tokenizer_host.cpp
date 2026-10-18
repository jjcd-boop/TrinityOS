#include "trinity_html5_tokenizer.hpp"
#include <cstdio>
#include <cstring>
using namespace trinity::html5;
static int failures = 0;
static void check(bool valid, const char* message) {
    if (!valid) { std::printf("FAIL: %s\n", message); ++failures; }
}
static bool equal(const char* lhs, const char* rhs) {
    return std::strcmp(lhs, rhs) == 0;
}
int main() {
    {
        const char input[] = "<!doctype html><div ID='x' id='y' data-q='a &amp; b' disabled><span>Hello &lt;world&gt;</span></div>";
        Tokenizer reader(input, sizeof(input)-1);
        Token t=reader.next();
        check(t.kind==Kind::Doctype && equal(t.name,"html"),"doctype");
        t=reader.next();
        check(t.kind==Kind::StartTag && equal(t.name,"div"),"div start");
        check(t.attr_count==3,"first duplicate attribute wins");
        check(equal(t.attrs[0].name,"id") && equal(t.attrs[0].value,"x"),"normalized id");
        check(equal(t.attrs[1].value,"a & b"),"decoded attribute reference");
        check(equal(t.attrs[2].name,"disabled") && !t.attrs[2].has_value,"boolean attribute");
        t=reader.next();check(t.kind==Kind::StartTag && equal(t.name,"span"),"span");
        t=reader.next();check(t.kind==Kind::Text && equal(t.text,"Hello <world>"),"text references");
        t=reader.next();check(t.kind==Kind::EndTag && equal(t.name,"span"),"span end");
        t=reader.next();check(t.kind==Kind::EndTag && equal(t.name,"div"),"div end");
        t=reader.next();check(t.kind==Kind::Eof,"eof");
    }
    {
        const char input[]="<script>if (a < b) s='&amp;';</script><style>.a{content:'<x>'}</style>";
        Tokenizer reader(input,sizeof(input)-1);
        Token t=reader.next();check(equal(t.name,"script"),"script start");
        t=reader.next();check(t.kind==Kind::Text && equal(t.text,"if (a < b) s='&amp;';"),"script raw bytes");
        t=reader.next();check(t.kind==Kind::EndTag && equal(t.name,"script"),"script end");
        t=reader.next();check(equal(t.name,"style"),"style start");
        t=reader.next();check(t.kind==Kind::Text && equal(t.text,".a{content:'<x>'}"),"style raw text");
        t=reader.next();check(t.kind==Kind::EndTag && equal(t.name,"style"),"style end");
    }
    {
        const char input[]="<textarea>one &amp; two <span>not markup</span></textarea>";
        Tokenizer reader(input,sizeof(input)-1);
        reader.next();
        Token t=reader.next();
        check(t.kind==Kind::Text && equal(t.text,"one & two <span>not markup</span>"),"RCDATA");
        t=reader.next();check(t.kind==Kind::EndTag && equal(t.name,"textarea"),"RCDATA close");
    }
    {
        const char input[]="<img alt=\"a > b\" src='abc.png' />";
        Tokenizer reader(input,sizeof(input)-1);
        Token t=reader.next();
        check(t.self_closing && t.attr_count==2,"self closing tag");
        check(equal(t.attrs[0].value,"a > b"),"quoted greater-than");
    }
    {
        const char input[]="A&#x1F600;&#128512;&#x80;B";
        Tokenizer reader(input,sizeof(input)-1);
        Token t=reader.next();
        const char expected[]="A\xF0\x9F\x98\x80\xF0\x9F\x98\x80\xE2\x82\xAC" "B";
        check(t.kind==Kind::Text && std::memcmp(t.text,expected,sizeof(expected))==0,"numeric references to UTF8");
    }
    {
        const char input[]="<!-- hello --><?bad?><p>x</p>";
        Tokenizer reader(input,sizeof(input)-1);
        Token t=reader.next();check(t.kind==Kind::Comment && equal(t.text," hello "),"comment");
        t=reader.next();check(t.kind==Kind::Comment && equal(t.text,"bad?"),"bogus comment");
        t=reader.next();check(t.kind==Kind::StartTag && equal(t.name,"p"),"tag after comment");
    }
    {
        const char input[]="<div id=x id=z>hi &unknown; end</div>";
        Tokenizer reader(input,sizeof(input)-1);
        Token t=reader.next();check(t.attr_count==1 && equal(t.attrs[0].value,"x"),"duplicate first");
        t=reader.next();check(equal(t.text,"hi &unknown; end"),"unknown entity kept literal");
    }
    if (failures) return 1;
    std::puts("PASS HTML5 tokenizer H1 host tests");
    return 0;
}
