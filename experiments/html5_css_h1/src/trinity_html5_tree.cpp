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
    active_count_=0;
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
bool TreeBuilder::is_formatting(const char* n) noexcept {
    return equal(n,"a") || equal(n,"b") || equal(n,"big") ||
        equal(n,"code") || equal(n,"em") || equal(n,"font") ||
        equal(n,"i") || equal(n,"s") || equal(n,"small") ||
        equal(n,"strike") || equal(n,"strong") || equal(n,"tt") ||
        equal(n,"u") || equal(n,"nobr");
}
bool TreeBuilder::is_special_block(const char* n) noexcept {
    // Restricted H3B1 furthest-block set; full WHATWG 'special' category
    // contains significantly more entries and requires full insertion modes.
    return is_block(n) || equal(n,"center") || equal(n,"fieldset") ||
        equal(n,"summary");
}
bool TreeBuilder::on_open_stack(uint16_t node,size_t* position) const noexcept {
    for(size_t i=0;i<depth_;++i){
        if(open_[i]==node){if(position)*position=i;return true;}
    }
    return false;
}
int TreeBuilder::active_last(const char* name) const noexcept {
    for(size_t i=active_count_;i>0;--i){
        const uint16_t index=active_[i-1];
        if(index==NO)break;
        if(index<count_ && equal(nodes_[index].name,name))
            return static_cast<int>(i-1);
    }
    return -1;
}
void TreeBuilder::active_remove_at(size_t position) noexcept {
    if(position>=active_count_)return;
    for(size_t i=position+1;i<active_count_;++i)
        active_[i-1]=active_[i];
    --active_count_;
}
void TreeBuilder::active_marker() noexcept {
    if(active_count_>=kFormattingLimit){error_=BuildError::FormattingLimit;return;}
    active_[active_count_++]=NO;
}
void TreeBuilder::active_clear_marker() noexcept {
    while(active_count_)if(active_[--active_count_]==NO)break;
}
bool TreeBuilder::active_add(uint16_t node) noexcept {
    if(node>=count_)return false;
    // Noah's Ark clause (bounded): at most three identical tag+attribute
    // signatures between consecutive markers.
    size_t matches=0,oldest=kFormattingLimit;
    for(size_t i=active_count_;i>0;--i){
        uint16_t a=active_[i-1];
        if(a==NO)break;
        const TreeNode& lhs=nodes_[a],& rhs=nodes_[node];
        if(!equal(lhs.name,rhs.name)||lhs.attr_count!=rhs.attr_count)continue;
        bool identical=true;
        for(uint8_t j=0;j<lhs.attr_count;++j){
            if(!equal(lhs.attrs[j].name,rhs.attrs[j].name) ||
               !equal(lhs.attrs[j].value,rhs.attrs[j].value) ||
               lhs.attrs[j].has_value!=rhs.attrs[j].has_value){
                identical=false;break;
            }
        }
        if(identical){++matches;oldest=i-1;}
    }
    if(matches>=3)active_remove_at(oldest);
    if(active_count_>=kFormattingLimit){error_=BuildError::FormattingLimit;return false;}
    active_[active_count_++]=node;
    return true;
}
uint16_t TreeBuilder::clone_element(uint16_t source,uint16_t parent) noexcept {
    if(source>=count_ || nodes_[source].kind!=NodeKind::Element)return NO;
    const uint16_t index=create(NodeKind::Element,parent,nodes_[source].name,nullptr,false);
    if(index==NO)return NO;
    TreeNode& target=nodes_[index];
    const TreeNode& original=nodes_[source];
    target.attr_count=original.attr_count;
    for(uint8_t i=0;i<target.attr_count;++i)target.attrs[i]=original.attrs[i];
    return index;
}
bool TreeBuilder::reconstruct_formatting() noexcept {
    if(!active_count_ || active_[active_count_-1]==NO ||
       on_open_stack(active_[active_count_-1]))return true;
    size_t first=active_count_-1;
    while(first>0 && active_[first-1]!=NO &&
          !on_open_stack(active_[first-1]))--first;
    for(size_t i=first;i<active_count_;++i){
        if(active_[i]==NO || on_open_stack(active_[i]))continue;
        uint16_t parent=open_[depth_-1];
        uint16_t before=NO;
        // Active entries may outlive malformed table tokens: use the same
        // adjusted insertion parent as the normal H3A element insertion.
        foster_location(parent,before);
        const uint16_t clone=clone_element(active_[i],parent);
        if(clone==NO)return false;
        if(before!=NO && !relocate(clone,parent,before))return false;
        if(!push(clone))return false;
        active_[i]=clone;
    }
    return true;
}
bool TreeBuilder::relocate(uint16_t node,uint16_t parent,uint16_t before) noexcept {
    if(node==0 || node>=count_ || parent>=count_ || node==parent ||
       before==node || (before!=NO &&
       (before>=count_||nodes_[before].parent!=parent)))return false;
    // Reject moves into the node's own subtree.
    for(uint16_t p=parent,guard=0;p!=NO && guard<count_;++guard){
        if(p==node)return false;
        p=nodes_[p].parent;
    }
    const uint16_t old=nodes_[node].parent;
    if(old==NO || old>=count_)return false;
    uint16_t previous=NO,current=nodes_[old].first_child;
    while(current!=NO && current!=node){
        previous=current;current=nodes_[current].next_sibling;
    }
    if(current!=node)return false;
    const uint16_t next=nodes_[node].next_sibling;
    if(previous==NO)nodes_[old].first_child=next;
    else nodes_[previous].next_sibling=next;
    if(nodes_[old].last_child==node)nodes_[old].last_child=previous;
    nodes_[node].parent=parent;nodes_[node].next_sibling=NO;
    uint16_t predecessor=previous_sibling(parent,before);
    if(before==NO){
        if(nodes_[parent].last_child!=NO)
            nodes_[nodes_[parent].last_child].next_sibling=node;
        else nodes_[parent].first_child=node;
        nodes_[parent].last_child=node;
    } else {
        nodes_[node].next_sibling=before;
        if(predecessor==NO)nodes_[parent].first_child=node;
        else nodes_[predecessor].next_sibling=node;
    }
    return true;
}
bool TreeBuilder::move_children(uint16_t source,uint16_t target,uint16_t except) noexcept {
    if(source>=count_ || target>=count_)return false;
    // Source children are moved in order, except a newly cloned target node.
    for(size_t guard=0;guard<count_;++guard){
        const uint16_t first=nodes_[source].first_child;
        if(first==NO || first==except)return true;
        if(!relocate(first,target,NO))return false;
    }
    error_=BuildError::TokenLimit;
    return false;
}
bool TreeBuilder::adopt_formatting_end(const char* name) noexcept {
    const int slot=active_last(name);
    if(slot<0){close_open(name);return true;}
    const uint16_t fmt=active_[static_cast<size_t>(slot)];
    size_t pos=0;
    if(!on_open_stack(fmt,&pos)){
        active_remove_at(static_cast<size_t>(slot));return true;
    }
    // The full algorithm handles complex intermediate formatting nodes,
    // special element scopes, markers, and a bookmark. This first bounded
    // implementation performs its no-furthest-block branch and direct-child
    // furthest-block repair. Complex cases conservatively close the stack.
    size_t furthest_pos=depth_;
    for(size_t i=pos+1;i<depth_;++i){
        if(is_special_block(nodes_[open_[i]].name)){furthest_pos=i;break;}
    }
    if(furthest_pos==depth_){
        depth_=pos;
        active_remove_at(static_cast<size_t>(slot));
        return true;
    }
    const uint16_t furthest=open_[furthest_pos];
    if(nodes_[furthest].parent!=fmt || pos+1!=furthest_pos){
        depth_=pos;
        active_remove_at(static_cast<size_t>(slot));
        return true;
    }
    const uint16_t parent=nodes_[fmt].parent;
    const uint16_t before=nodes_[fmt].next_sibling;
    if(!relocate(furthest,parent,before))return false;
    const uint16_t clone=clone_element(fmt,furthest);
    if(clone==NO || !move_children(furthest,clone,clone))return false;
    // Continue parsing within the open furthest block; stale descendants
    // are no longer current but other active formatting entries can rebuild.
    depth_=pos;open_[depth_++]=furthest;
    active_remove_at(static_cast<size_t>(slot));
    return true;
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
bool TreeBuilder::close_open_in_table(const char* name) noexcept {
    for(size_t i=depth_;i>1;--i) {
        const char* current=nodes_[open_[i-1]].name;
        // The first enclosing table is the boundary. A row or cell in an
        // outer table is not in scope for the currently nested table.
        if(equal(current,"table"))break;
        if(equal(current,name)){depth_=i-1;return true;}
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
    if(equal(n,"tr"))close_open_in_table("tr");
    if(equal(n,"td")||equal(n,"th")) {close_open_in_table("td");close_open_in_table("th");}
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
    if(equal(t.name,"tr")||equal(t.name,"td")||equal(t.name,"th")||
       equal(t.name,"tbody")||equal(t.name,"thead")||equal(t.name,"tfoot"))
        close_open_in_table(t.name);
    else close_open(t.name);
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
