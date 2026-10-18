#include "trinity_html5_tree.hpp"
#include <cstdio>
#include <cstring>
#include <string>

using namespace trinity::html5;
static TreeBuilder doc; // Large bounded arena belongs outside a small runtime stack.
static int failures=0;
static constexpr uint16_t NONE=TreeNode::None;

static void check(bool ok,const char* label) {
    if(!ok){std::printf("FAIL %s\n",label);++failures;}
}
static bool eq(const char* a,const char* b) {
    return a && b && std::strcmp(a,b)==0;
}
static uint16_t child(uint16_t parent,const char* name) {
    return doc.first_named_child(parent,name);
}
static uint16_t next(uint16_t index) {
    const TreeNode* n=doc.get(index);return n?n->next_sibling:NONE;
}
static uint16_t first(uint16_t index) {
    const TreeNode* n=doc.get(index);return n?n->first_child:NONE;
}
static bool has_text(uint16_t index,const char* value) {
    const TreeNode* n=doc.get(index);
    return n && n->kind==NodeKind::Text && eq(n->text,value);
}
static void start(const char* html) {
    if(!doc.build(html,std::strlen(html))) {
        std::printf("FAILED BUILD %s status=%d\n",html,static_cast<int>(doc.error()));
        ++failures;
    }
}
int main() {
    {
        start("<div>Hello <b>world</b></div>");
        check(doc.html_index()!=NONE && doc.head_index()!=NONE && doc.body_index()!=NONE,
              "implied html head and body");
        check(doc.get(doc.html_index())->implied,"implied html flag");
        check(doc.get(doc.head_index())->implied && doc.get(doc.body_index())->implied,
              "implied head body flags");
        uint16_t div=child(doc.body_index(),"div");
        uint16_t b=child(div,"b");
        check(div!=NONE && b!=NONE,"DOM nested elements");
        check(has_text(first(div),"Hello "),"DOM text");
        check(has_text(first(b),"world"),"nested text");
    }
    {
        start("<!doctype html><html lang=en><head><title>T &amp; T</title></head><body id=page><p>Hi</p></body></html>");
        check(doc.get(first(0))->kind==NodeKind::Doctype,"doctype root");
        const TreeNode* html=doc.get(doc.html_index());
        check(html && !html->implied && html->attr_count==1 &&
              eq(html->attrs[0].value,"en"),"explicit html attributes");
        check(!doc.get(doc.head_index())->implied &&
              !doc.get(doc.body_index())->implied,"explicit head/body");
        uint16_t title=child(doc.head_index(),"title");
        check(title!=NONE && has_text(first(title),"T & T"),"RCDATA in head");
        uint16_t p=child(doc.body_index(),"p");
        check(p!=NONE && has_text(first(p),"Hi"),"body text preserved");
    }
    {
        start("<p>first<p>second");
        uint16_t p=child(doc.body_index(),"p");
        check(p!=NONE && has_text(first(p),"first"),"first p");
        uint16_t next_p=next(p);
        check(next_p!=NONE && eq(doc.get(next_p)->name,"p") &&
              has_text(first(next_p),"second"),"implied p end");
    }
    {
        start("<div><p>paragraph<div>block</div>last</div>");
        uint16_t div=child(doc.body_index(),"div");
        uint16_t p=child(div,"p");
        uint16_t sibling=next(p);
        check(p!=NONE && has_text(first(p),"paragraph"),"p inside block");
        check(sibling!=NONE && eq(doc.get(sibling)->name,"div") &&
              has_text(first(sibling),"block"),"block closes p");
        check(has_text(next(sibling),"last"),"text after nested block");
    }
    {
        start("<ul><li>red<li>blue<li>green</ul>");
        uint16_t ul=child(doc.body_index(),"ul");
        uint16_t li=child(ul,"li");
        check(li!=NONE && has_text(first(li),"red"),"li one");
        li=next(li);check(li!=NONE && has_text(first(li),"blue"),"li two");
        li=next(li);check(li!=NONE && has_text(first(li),"green"),"li three");
    }
    {
        start("<dl><dt>term<dd>description</dl>");
        uint16_t dl=child(doc.body_index(),"dl");
        uint16_t dt=child(dl,"dt");
        uint16_t dd=next(dt);
        check(dt!=NONE && has_text(first(dt),"term"),"dt optional end");
        check(dd!=NONE && eq(doc.get(dd)->name,"dd") &&
              has_text(first(dd),"description"),"dd sibling");
    }
    {
        start("<table><tr><td>A<td>B</tr></table>");
        uint16_t table=child(doc.body_index(),"table");
        uint16_t tbody=child(table,"tbody");
        uint16_t tr=child(tbody,"tr");
        uint16_t cell=child(tr,"td");
        check(tbody!=NONE && doc.get(tbody)->implied,"implied tbody");
        check(tr!=NONE && doc.get(tr)->implied==false,"explicit tr");
        check(cell!=NONE && has_text(first(cell),"A"),"first cell");
        cell=next(cell);
        check(cell!=NONE && eq(doc.get(cell)->name,"td") &&
              has_text(first(cell),"B"),"auto-closed table cell");
    }
    {
        start("<div><img src='x.png'><br><input disabled>text</div>");
        uint16_t div=child(doc.body_index(),"div");
        uint16_t img=child(div,"img");
        uint16_t br=next(img);
        uint16_t input=next(br);
        check(img!=NONE && first(img)==NONE && eq(doc.get(img)->attrs[0].value,"x.png"),"void image");
        check(br!=NONE && eq(doc.get(br)->name,"br") && first(br)==NONE,"void br");
        check(input!=NONE && eq(doc.get(input)->name,"input") &&
              doc.get(input)->attrs[0].has_value==false,"boolean attribute");
        check(has_text(next(input),"text"),"text after void elements");
    }
    {
        start("<script>if (x < 4) run('&amp;');</script><p>next</p>");
        uint16_t script=child(doc.head_index(),"script");
        check(script!=NONE && has_text(first(script),"if (x < 4) run('&amp;');"),
              "head script raw text");
        uint16_t p=child(doc.body_index(),"p");
        check(p!=NONE && has_text(first(p),"next"),"body after script");
    }
    {
        start("<div id=x id=y class='a' title='hello'>text</div>");
        uint16_t div=child(doc.body_index(),"div");
        const TreeNode* node=doc.get(div);
        check(node && node->attr_count==3 && eq(node->attrs[0].value,"x"),
              "duplicate attribute discarded in tree");
        check(eq(node->attrs[2].value,"hello"),"attributes retained");
    }
    {
        start("<p/><span>nested</span></p>");
        uint16_t p=child(doc.body_index(),"p");
        check(p!=NONE && child(p,"span")!=NONE,
              "HTML self-closing flag ignored on nonvoid element");
    }
    {
        start("<div>a</div><!--comment--><div>b</div>");
        uint16_t d=child(doc.body_index(),"div");
        uint16_t comment=next(d);
        check(comment!=NONE && doc.get(comment)->kind==NodeKind::Comment &&
              eq(doc.get(comment)->text,"comment"),"body comment");
        uint16_t second=next(comment);
        check(second!=NONE && eq(doc.get(second)->name,"div"),"sibling after comment");
    }
    {
        start("</p>tail");
        uint16_t p=child(doc.body_index(),"p");
        check(p!=NONE && first(p)==NONE && has_text(next(p),"tail"),
              "unmatched end p synthesizes empty p");
    }
    {
        // H1 emits <=1023 byte text tokens. H2 must preserve all the bytes,
        // splitting them into adjacent text nodes rather than truncating.
        std::string big="<div>";
        big += std::string(700,'x');
        big += "</div>";
        if(!doc.build(big.data(),big.size())) {
            check(false,"long text parsed");
        } else {
            uint16_t div=child(doc.body_index(),"div");
            size_t total=0;
            for(uint16_t n=first(div);n!=NONE;n=next(n)) {
                check(doc.get(n)->kind==NodeKind::Text,"fragment text node");
                total+=doc.get(n)->text_len;
            }
            check(total==700,"no text loss across node limit");
        }
    }
    {
        check(doc.build("",0),"empty input accepted");
        check(doc.count()==4 && doc.html_index()!=NONE &&
              doc.head_index()!=NONE && doc.body_index()!=NONE,
              "empty document implied skeleton");
    }
    {
        const char input[]="<a a=1 b=2 c=3 d=4 e=5 f=6 g=7>too many attributes</a>";
        check(!doc.build(input,sizeof(input)-1) &&
              doc.error()==BuildError::AttributeLimit,
              "fail closed when attribute capacity exceeded");
    }
    {
        std::string deep;
        for(size_t i=0;i<72;++i)deep+="<div>";
        check(!doc.build(deep.data(),deep.size()) &&
              doc.error()==BuildError::DepthLimit,
              "bounded open-element stack");
    }
    if(failures){std::printf("FAIL H2 DOM tree: %d cases\n",failures);return 1;}
    std::puts("PASS HTML5 H2 DOM tree construction host tests");
    return 0;
}
