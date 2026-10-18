#include "trinity_html5_tree.hpp"

namespace trinity::html5 {
static constexpr uint16_t NO = TreeNode::None;

TreeBuilder::TreeBuilder() noexcept { reset(); }

void TreeBuilder::reset() noexcept {
    count_ = 1; depth_ = 1; open_[0] = 0;
    nodes_[0] = TreeNode{};
    nodes_[0].kind = NodeKind::Document;
    html_ = head_ = body_ = NO;
    mode_ = Mode::BeforeHtml;
    error_ = BuildError::None;
}
bool TreeBuilder::equal(const char* a, const char* b) noexcept {
    if (!a || !b) return false;
    while (*a && *b && *a == *b) { ++a; ++b; }
    return *a == 0 && *b == 0;
}
bool TreeBuilder::whitespace_only(const char* text, size_t n) noexcept {
    for (size_t i = 0; i < n; ++i) {
        const char c = text[i];
        if (c != ' ' && c != '\n' && c != '\r' && c != '\t' && c != '\f') return false;
    }
    return true;
}
void TreeBuilder::copy(char* dst, size_t cap, const char* src, bool& overflow) noexcept {
    if (!cap) { overflow = true; return; }
    size_t i = 0;
    if (src) {
        while (src[i] && i+1 < cap) { dst[i] = src[i]; ++i; }
        if (src[i]) overflow = true;
    }
    dst[i] = '\0';
}
bool TreeBuilder::is_head_tag(const char* n) noexcept {
    return equal(n,"title") || equal(n,"meta") || equal(n,"link") ||
        equal(n,"base") || equal(n,"style") || equal(n,"script") ||
        equal(n,"noscript") || equal(n,"template");
}
bool TreeBuilder::is_void(const char* n) noexcept {
    return equal(n,"area") || equal(n,"base") || equal(n,"br") ||
        equal(n,"col") || equal(n,"embed") || equal(n,"hr") ||
        equal(n,"img") || equal(n,"input") || equal(n,"link") ||
        equal(n,"meta") || equal(n,"param") || equal(n,"source") ||
        equal(n,"track") || equal(n,"wbr") || equal(n,"keygen");
}
bool TreeBuilder::is_heading(const char* n) noexcept {
    return n && n[0]=='h' && n[1]>='1' && n[1]<='6' && !n[2];
}
bool TreeBuilder::is_block(const char* n) noexcept {
    return equal(n,"div") || equal(n,"section") || equal(n,"article") ||
        equal(n,"header") || equal(n,"footer") || equal(n,"main") ||
        equal(n,"aside") || equal(n,"nav") || equal(n,"ul") ||
        equal(n,"ol") || equal(n,"li") || equal(n,"dl") ||
        equal(n,"p") || equal(n,"pre") || equal(n,"blockquote") ||
        equal(n,"table") || equal(n,"form") || equal(n,"hgroup") ||
        is_heading(n) || equal(n,"address") || equal(n,"figure") ||
        equal(n,"figcaption") || equal(n,"hr") || equal(n,"details");
}
uint16_t TreeBuilder::previous_sibling(uint16_t parent, uint16_t before) const noexcept {
    if (parent>=count_) return NO;
    if (before==NO) return nodes_[parent].last_child;
    if (before>=count_ || nodes_[before].parent!=parent) return NO;
    uint16_t current=nodes_[parent].first_child, previous=NO;
    // Bounded traversal also prevents an infinite loop on corrupted links.
    for (size_t i=0;i<count_ && current!=NO;++i) {
        if (current==before) return previous;
        previous=current;
        current=nodes_[current].next_sibling;
    }
    return NO;
}
uint16_t TreeBuilder::create(NodeKind kind, uint16_t parent, const char* name,
                             const Token* token, bool implied, uint16_t before) noexcept {
    if (count_ >= kMaxNodes || parent >= count_) {
        error_ = BuildError::NodeLimit; return NO;
    }
    if (before!=NO && (before>=count_ || nodes_[before].parent!=parent)) {
        error_ = BuildError::TokenOverflow; return NO;
    }
    if (token && token->attr_count > 6) {
        error_ = BuildError::AttributeLimit; return NO;
    }
    const uint16_t predecessor=previous_sibling(parent,before);
    const uint16_t index = static_cast<uint16_t>(count_++);
    TreeNode& n = nodes_[index]; n = TreeNode{};
    n.kind = kind; n.parent = parent; n.implied = implied;
    bool overflow = false;
    copy(n.name, sizeof(n.name), name, overflow);
    if (token) {
        n.attr_count = token->attr_count;
        for (uint8_t i = 0; i < n.attr_count; ++i) {
            copy(n.attrs[i].name,sizeof(n.attrs[i].name),token->attrs[i].name,overflow);
            copy(n.attrs[i].value,sizeof(n.attrs[i].value),token->attrs[i].value,overflow);
            n.attrs[i].has_value = token->attrs[i].has_value;
        }
    }
    if (overflow) { error_ = BuildError::AttributeLimit; return NO; }
    TreeNode& p=nodes_[parent];
    if(before==NO) {
        if(p.last_child!=NO) nodes_[p.last_child].next_sibling=index;
        else p.first_child=index;
        p.last_child=index;
    } else {
        n.next_sibling=before;
        if(predecessor==NO) p.first_child=index;
        else nodes_[predecessor].next_sibling=index;
    }
    return index;
}
bool TreeBuilder::push(uint16_t index) noexcept {
    if (index == NO) return false;
    if (depth_ >= kMaxDepth) { error_ = BuildError::DepthLimit; return false; }
    open_[depth_++] = index;
    return true;
}
bool TreeBuilder::enter_html(const Token* t) noexcept {
    if (html_ != NO) return true;
    html_ = create(NodeKind::Element,0,"html",t,t==nullptr);
    if (!push(html_)) return false;
    mode_ = Mode::BeforeHead;
    return true;
}
bool TreeBuilder::enter_head(const Token* t) noexcept {
    if (head_ != NO) return true;
    head_ = create(NodeKind::Element,html_,"head",t,t==nullptr);
    if (!push(head_)) return false;
    mode_ = Mode::InHead;
    return true;
}
bool TreeBuilder::finish_head() noexcept {
    if (head_ == NO && !enter_head(nullptr)) return false;
    for (size_t i=depth_; i>0; --i) {
        if (open_[i-1]==head_) {
            depth_=i-1;
            mode_=Mode::AfterHead;
            return true;
        }
    }
    mode_=Mode::AfterHead;
    return true;
}
bool TreeBuilder::enter_body(const Token* t) noexcept {
    if (body_!=NO) return true;
    if (head_==NO && !enter_head(nullptr)) return false;
    if (!finish_head()) return false;
    body_=create(NodeKind::Element,html_,"body",t,t==nullptr);
    if(!push(body_)) return false;
    mode_=Mode::InBody;
    return true;
}
bool TreeBuilder::append_text(uint16_t parent,const char* s,size_t n,
                              uint16_t before) noexcept {
    if (parent>=count_ || !s) return false;
    if (before!=NO && (before>=count_ || nodes_[before].parent!=parent)) {
        error_=BuildError::TokenOverflow;return false;
    }
    size_t offset=0;
    while (offset<n) {
        uint16_t idx=previous_sibling(parent,before);
        if (idx==NO || nodes_[idx].kind!=NodeKind::Text ||
            nodes_[idx].text_len>=sizeof(nodes_[idx].text)-1) {
            idx=create(NodeKind::Text,parent,nullptr,nullptr,false,before);
            if(idx==NO) return false;
        }
        TreeNode& target=nodes_[idx];
        size_t available=sizeof(target.text)-1-target.text_len;
        size_t add=n-offset;
        if(add>available) add=available;
        for(size_t j=0;j<add;++j) target.text[target.text_len+j]=s[offset+j];
        target.text_len=static_cast<uint16_t>(target.text_len+add);
        target.text[target.text_len]='\0';
        offset+=add;
    }
    return true;
}
bool TreeBuilder::foster_location(uint16_t& parent, uint16_t& before) const noexcept {
    if(depth_<=1)return false;
    const char* current=nodes_[open_[depth_-1]].name;
    if(!equal(current,"table") && !equal(current,"tbody") &&
       !equal(current,"thead") && !equal(current,"tfoot") &&
       !equal(current,"tr"))return false;
    // The most recently opened table is the foster-parenting anchor;
    // do not cross a nested table or leave the document's owned arena.
    for(size_t i=depth_;i>1;--i) {
        const uint16_t index=open_[i-1];
        if(equal(nodes_[index].name,"table")) {
            const uint16_t owner=nodes_[index].parent;
            if(owner==NO)return false;
            parent=owner;before=index;
            return true;
        }
    }
    return false;
}
bool TreeBuilder::append_token_text(const Token& t) noexcept {
    if (t.text_len==0) return true;
    uint16_t parent=open_[depth_-1],before=NO;
    // The HTML "in table text" insertion mode fosters non-whitespace
    // character tokens. Whitespace stays with the table scaffold.
    if(!whitespace_only(t.text,t.text_len))
        foster_location(parent,before);
    return append_text(parent,t.text,t.text_len,before);
}
bool TreeBuilder::append_comment(const Token& t) noexcept {
    const uint16_t parent=mode_==Mode::BeforeHtml ? 0 : open_[depth_-1];
    const uint16_t index=create(NodeKind::Comment,parent,nullptr,nullptr,false);
    if(index==NO) return false;
    const size_t n=t.text_len;
    if(n>=sizeof(nodes_[index].text)) { error_=BuildError::TokenOverflow; return false; }
    for(size_t j=0;j<n;++j) nodes_[index].text[j]=t.text[j];
    nodes_[index].text[n]=0;nodes_[index].text_len=static_cast<uint16_t>(n);
    return true;
}
bool TreeBuilder::close_open(const char* name) noexcept {
    for(size_t i=depth_;i>1;--i) {
        const uint16_t candidate=open_[i-1];
        if(equal(nodes_[candidate].name,name)) { depth_=i-1; return true; }
    }
    return false;
}
bool TreeBuilder::close_heading() noexcept {
    for(size_t i=depth_;i>1;--i)
        if(is_heading(nodes_[open_[i-1]].name)) { depth_=i-1;return true; }
    return false;
}
bool TreeBuilder::close_list_item(const char* name) noexcept {
    for(size_t i=depth_;i>1;--i) {
        const char* current=nodes_[open_[i-1]].name;
        if(equal(current,name) ||
            ((equal(name,"dd")||equal(name,"dt")) && (equal(current,"dd")||equal(current,"dt")))) {
            depth_=i-1;return true;
        }
        if(equal(current,"ul") || equal(current,"ol") || equal(current,"dl") ||
           equal(current,"body")) break;
    }
    return false;
}
bool TreeBuilder::ensure_table_context(const char* name) noexcept {
    if(!equal(name,"tr") && !equal(name,"td") && !equal(name,"th")) return true;
    size_t table=0;
    for(size_t i=depth_;i>1;--i) {
        if(equal(nodes_[open_[i-1]].name,"table")) { table=i;break; }
    }
    if(!table) return true;
    bool rowgroup=false;
    bool row=false;
    for(size_t i=table;i<depth_;++i) {
        const char* tag=nodes_[open_[i]].name;
        if(equal(tag,"thead")||equal(tag,"tbody")||equal(tag,"tfoot")) rowgroup=true;
        if(equal(tag,"tr")) row=true;
    }
    if(!rowgroup) {
        uint16_t id=create(NodeKind::Element,open_[depth_-1],"tbody",nullptr,true);
        if(!push(id))return false;
    }
    if((equal(name,"td")||equal(name,"th")) && !row) {
        uint16_t id=create(NodeKind::Element,open_[depth_-1],"tr",nullptr,true);
        if(!push(id))return false;
    }
    return true;
}
bool TreeBuilder::start_body_tag(const Token& t) noexcept {
    const char* n=t.name;
    if(equal(n,"html")||equal(n,"body")||equal(n,"head")) return true;
    if(equal(n,"p"))close_open("p");
    else if(equal(n,"li"))close_list_item("li");
    else if(equal(n,"dt")||equal(n,"dd"))close_list_item(n);
    else if(is_heading(n))close_heading();
    else if(equal(n,"a"))close_open("a");
    else if(is_block(n))close_open("p");
    if(equal(n,"tr"))close_open("tr");
    if(equal(n,"td")||equal(n,"th")) {close_open("td");close_open("th");}
    if(!ensure_table_context(n))return false;
    uint16_t parent=open_[depth_-1], before=NO;
    // Non-table children encountered directly in a table scaffold belong
    // immediately before the table in its owner's child sequence.
    const bool is_table_structure =
        equal(n,"caption") || equal(n,"colgroup") || equal(n,"col") ||
        equal(n,"tbody") || equal(n,"thead") || equal(n,"tfoot") ||
        equal(n,"tr") || equal(n,"td") || equal(n,"th");
    if(!is_table_structure)foster_location(parent,before);
    const uint16_t id=create(NodeKind::Element,parent,n,&t,false,before);
    if(id==NO)return false;
    // In HTML, a self-closing slash on an ordinary (non-void) element is
    // ignored. A later foreign-content pass will implement SVG/MathML flags.
    return is_void(n) ? true : push(id);
}
bool TreeBuilder::end_body_tag(const Token& t) noexcept {
    if(equal(t.name,"html")) { mode_=Mode::AfterBody; return true; }
    if(equal(t.name,"body")) { mode_=Mode::AfterBody;return true; }
    if(equal(t.name,"head"))return true;
    if(equal(t.name,"p")) {
        bool found=false;
        for(size_t i=depth_;i>1;--i)
            if(equal(nodes_[open_[i-1]].name,"p")){found=true;break;}
        if(!found){
            Token synthetic{};synthetic.kind=Kind::StartTag;
            synthetic.name[0]='p';synthetic.name[1]=0;
            if(!start_body_tag(synthetic))return false;
        }
    }
    close_open(t.name);
    return true;
}
bool TreeBuilder::in_body(const Token& t) noexcept {
    if(t.kind==Kind::Text)return append_token_text(t);
    if(t.kind==Kind::Comment)return append_comment(t);
    if(t.kind==Kind::StartTag)return start_body_tag(t);
    if(t.kind==Kind::EndTag)return end_body_tag(t);
    return true;
}
bool TreeBuilder::handle(const Token& t) noexcept {
    if(t.overflow) { error_=BuildError::TokenOverflow;return false; }
    if(t.kind==Kind::Doctype){
        const uint16_t id=create(NodeKind::Doctype,0,t.name,nullptr,false);
        return id!=NO;
    }
    if(t.kind==Kind::Comment) return append_comment(t);
    // A token may be reprocessed when an implied html/head/body is created.
    // Each transition advances the mode; no recursion or heap allocations.
    for(unsigned retry=0;retry<6;++retry) {
        switch(mode_) {
        case Mode::BeforeHtml:
            if(t.kind==Kind::EndTag)return true;
            if(t.kind==Kind::Text && whitespace_only(t.text,t.text_len))return true;
            if(t.kind==Kind::StartTag && equal(t.name,"html"))
                return enter_html(&t);
            if(!enter_html(nullptr))return false;
            continue;
        case Mode::BeforeHead:
            if(t.kind==Kind::Text && whitespace_only(t.text,t.text_len))return true;
            if(t.kind==Kind::StartTag && equal(t.name,"head"))
                return enter_head(&t);
            if(t.kind==Kind::StartTag && is_head_tag(t.name)) {
                if(!enter_head(nullptr))return false;
                continue;
            }
            if(!enter_head(nullptr) || !finish_head())return false;
            continue;
        case Mode::InHead: {
            if(t.kind==Kind::EndTag && equal(t.name,"head"))return finish_head();
            if(depth_>0 && open_[depth_-1]!=head_) {
                // E.g. raw text inside an explicitly open script/title.
                return in_body(t);
            }
            if(t.kind==Kind::Text && whitespace_only(t.text,t.text_len))
                return append_token_text(t);
            if(t.kind==Kind::StartTag && is_head_tag(t.name))
                return start_body_tag(t);
            if(!finish_head())return false;
            continue;
        }
        case Mode::AfterHead:
            if(t.kind==Kind::StartTag && equal(t.name,"body"))
                return enter_body(&t);
            if(t.kind==Kind::Text && whitespace_only(t.text,t.text_len))return true;
            if(!enter_body(nullptr))return false;
            continue;
        case Mode::InBody:
            return in_body(t);
        case Mode::AfterBody:
            if(t.kind==Kind::EndTag && equal(t.name,"html"))return true;
            // Text/tags after </body> are repaired by in-body processing.
            mode_=Mode::InBody;
            return in_body(t);
        }
    }
    error_=BuildError::TokenLimit;
    return false;
}
bool TreeBuilder::build(const char* source,size_t length) noexcept {
    reset();
    if(!source && length){error_=BuildError::TokenOverflow;return false;}
    Tokenizer tok(source,length);
    for(size_t i=0;i<kMaxTokens;++i) {
        const Token t=tok.next();
        if(t.kind==Kind::Eof) {
            if(html_==NO && !enter_html(nullptr))return false;
            if(head_==NO && !enter_head(nullptr))return false;
            if(body_==NO && !enter_body(nullptr))return false;
            return error_==BuildError::None;
        }
        if(!handle(t))return false;
    }
    error_=BuildError::TokenLimit;
    return false;
}
uint16_t TreeBuilder::first_named_child(uint16_t parent,const char* name) const noexcept {
    if(parent>=count_)return NO;
    uint16_t child=nodes_[parent].first_child;
    for(size_t i=0;i<count_ && child!=NO;++i) {
        if(equal(nodes_[child].name,name))return child;
        child=nodes_[child].next_sibling;
    }
    return NO;
}
} // namespace trinity::html5
