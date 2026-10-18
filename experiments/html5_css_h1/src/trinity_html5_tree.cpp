#include "trinity_html5_tree.hpp"
#include "trinity_html5_segmented_nodes.hpp"

namespace trinity::html5 {
static constexpr uint16_t NO = TreeNode::None;

TreeNode& TreeBuilder::NodeAccess::operator[](size_t i) noexcept {
#ifdef TRINITY_HTML5_SEGMENTED_ONLY
    return *arena->get(static_cast<uint16_t>(i));
#else
    return arena ? *arena->get(static_cast<uint16_t>(i)) : fixed[i];
#endif
}
const TreeNode& TreeBuilder::NodeAccess::operator[](size_t i) const noexcept {
#ifdef TRINITY_HTML5_SEGMENTED_ONLY
    return *arena->get(static_cast<uint16_t>(i));
#else
    return arena ? *arena->get(static_cast<uint16_t>(i)) : fixed[i];
#endif
}

#ifndef TRINITY_HTML5_SEGMENTED_ONLY
TreeBuilder::TreeBuilder() noexcept { reset(); }
#endif
TreeBuilder::TreeBuilder(SegmentedNodeArena& storage) noexcept {
    nodes_.arena=&storage;
    reset();
}

const TreeNode* TreeBuilder::get(uint16_t i) const noexcept {
    if(i>=count_)return nullptr;
#ifdef TRINITY_HTML5_SEGMENTED_ONLY
    return nodes_.arena ? nodes_.arena->get(i) : nullptr;
#else
    return nodes_.arena ? nodes_.arena->get(i) : &nodes_.fixed[i];
#endif
}
size_t TreeBuilder::node_limit() const noexcept {
    return nodes_.arena ? nodes_.arena->node_budget() : kMaxNodes;
}
BuildError TreeBuilder::arena_error() const noexcept {
    if(!nodes_.arena)return BuildError::NodeLimit;
    switch(nodes_.arena->error()) {
        case NodeArenaError::AllocatorFailure: return BuildError::AllocatorFailure;
        case NodeArenaError::BadConfiguration:
        case NodeArenaError::ByteBudget: return BuildError::MemoryBudget;
        case NodeArenaError::NodeBudget: return BuildError::NodeLimit;
        case NodeArenaError::None: return BuildError::MemoryBudget;
    }
    return BuildError::MemoryBudget;
}
bool TreeBuilder::allocate_node(uint16_t& index) noexcept {
    index=NO;
    if(count_>=node_limit() || count_>=TreeNode::None) {
        error_=BuildError::NodeLimit;
        return false;
    }
    if(nodes_.arena) {
        uint16_t actual=NO;
        if(!nodes_.arena->append(TreeNode{},actual)){
            error_=arena_error();
            return false;
        }
        if(actual!=count_){
            error_=BuildError::MemoryBudget;
            return false;
        }
    }
    index=static_cast<uint16_t>(count_++);
    return true;
}
void TreeBuilder::reset() noexcept {
    count_=0;
    depth_=1;open_[0]=0;
    html_=head_=body_=NO;
    mode_=Mode::BeforeHtml;
    active_count_=0;
    pending_table_len_=0;
    pending_table_active_=false;
    error_=BuildError::None;
    if(nodes_.arena) {
        nodes_.arena->clear();
        uint16_t root=NO;
        if(!nodes_.arena->append(TreeNode{},root) || root!=0) {
            error_=arena_error();
            return;
        }
    }
    count_=1;
    nodes_[0]=TreeNode{};
    nodes_[0].kind=NodeKind::Document;
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
        equal(n,"figcaption") || equal(n,"hr") || equal(n,"details") ||
        equal(n,"center") || equal(n,"dir") ||
        equal(n,"fieldset") || equal(n,"menu") || equal(n,"search") ||
        equal(n,"summary") || equal(n,"listing");
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
int TreeBuilder::active_index(uint16_t node) const noexcept {
    for(size_t i=0;i<active_count_;++i)
        if(active_[i]==node)return static_cast<int>(i);
    return -1;
}
void TreeBuilder::stack_remove(size_t position) noexcept {
    if(position<2 || position>=depth_)return; // document and html never removed here
    for(size_t i=position+1;i<depth_;++i)open_[i-1]=open_[i];
    --depth_;
}
bool TreeBuilder::stack_insert(size_t position,uint16_t node) noexcept {
    if(position<2 || position>depth_ || node>=count_ ||
       depth_>=kMaxDepth){error_=BuildError::DepthLimit;return false;}
    for(size_t i=depth_;i>position;--i)open_[i]=open_[i-1];
    open_[position]=node;++depth_;
    return true;
}
bool TreeBuilder::active_insert(size_t position,uint16_t node) noexcept {
    if(position>active_count_ || node>=count_ ||
       active_count_>=kFormattingLimit){
        error_=BuildError::FormattingLimit;return false;
    }
    for(size_t i=active_count_;i>position;--i)active_[i]=active_[i-1];
    active_[position]=node;++active_count_;
    return true;
}
bool TreeBuilder::formatting_in_scope(size_t open_position) const noexcept {
    // This is a deliberately narrow approximation of the HTML "in scope"
    // predicate, including table-cell and template boundaries. The full
    // namespace-aware scope boundary table is a subsequent conformance pass.
    if(open_position<2 || open_position>=depth_)return false;
    for(size_t i=depth_;i>open_position+1;--i){
        const char* tag=nodes_[open_[i-1]].name;
        if(equal(tag,"table") || equal(tag,"td") || equal(tag,"th") ||
           equal(tag,"caption") || equal(tag,"template") ||
           equal(tag,"select"))return false;
    }
    return true;
}
bool TreeBuilder::adopt_formatting_end(const char* name) noexcept {
    // Adapted from WHATWG's adoption-agency *algorithmic requirements*, with
    // bounded memory and no DOM sanitizers, foreign content, or script
    // re-entrancy. This does not yet constitute a full HTML parser.
    if(depth_>2 && equal(nodes_[open_[depth_-1]].name,name) &&
       active_index(open_[depth_-1])<0){
        --depth_;
        return true;
    }
    for(size_t outer=0;outer<8;++outer){
        const int found=active_last(name);
        if(found<0){
            // "Any other end tag" fallback. The active-formatting list may
            // be cut off by a td/th/caption marker although a same-named
            // element remains beneath an inner table. Never close across
            // the first special element in the open-element stack.
            for(size_t i=depth_;i>1;--i){
                const char* top=nodes_[open_[i-1]].name;
                if(equal(top,name)){depth_=i-1;return true;}
                if(is_special_block(top) || equal(top,"td") ||
                   equal(top,"th") || equal(top,"tbody") ||
                   equal(top,"thead") || equal(top,"tfoot") ||
                   equal(top,"tr") || equal(top,"template") ||
                   equal(top,"select"))return true;
            }
            return true;
        }
        const uint16_t formatting=active_[static_cast<size_t>(found)];
        size_t format_pos=0;
        if(!on_open_stack(formatting,&format_pos)){
            active_remove_at(static_cast<size_t>(found));
            return true;
        }
        if(!formatting_in_scope(format_pos))return true;
        size_t furthest_pos=depth_;
        for(size_t i=format_pos+1;i<depth_;++i){
            if(is_special_block(nodes_[open_[i]].name)){
                furthest_pos=i;break;
            }
        }
        if(furthest_pos==depth_){
            depth_=format_pos;
            active_remove_at(static_cast<size_t>(found));
            return true;
        }
        const uint16_t furthest=open_[furthest_pos];
        const uint16_t common=open_[format_pos-1];
        uint16_t last_node=furthest;
        uint16_t bookmark_after=NO;
        // Inner loop traverses towards the formatting node. A node not in the
        // active list is removed only from the stack, not from the DOM.
        size_t inner=0;
        for(size_t i=furthest_pos;i>format_pos+1;){
            --i;++inner;
            const uint16_t current=open_[i];
            int active_slot=active_index(current);
            if(inner>3 && active_slot>=0){
                active_remove_at(static_cast<size_t>(active_slot));
                active_slot=-1;
            }
            if(active_slot<0){
                stack_remove(i);
                --furthest_pos;
                continue;
            }
            const uint16_t clone=clone_element(current,nodes_[current].parent);
            if(clone==NO)return false;
            active_[static_cast<size_t>(active_slot)]=clone;
            open_[i]=clone;
            if(bookmark_after==NO)bookmark_after=clone;
            if(!relocate(last_node,clone,NO)){
                error_=BuildError::TokenOverflow;return false;
            }
            last_node=clone;
        }

        uint16_t target=common,before=NO;
        const char* common_tag=nodes_[common].name;
        if(equal(common_tag,"table") || equal(common_tag,"tbody") ||
           equal(common_tag,"thead") || equal(common_tag,"tfoot") ||
           equal(common_tag,"tr")){
            const uint16_t owner=nodes_[common].parent;
            if(owner!=NO){target=owner;before=common;}
        }
        if(!relocate(last_node,target,before)){
            error_=BuildError::TokenOverflow;return false;
        }
        const uint16_t replacement=clone_element(formatting,furthest);
        if(replacement==NO || !move_children(furthest,replacement,replacement))
            return false;

        // The bookmark belongs immediately after the first intermediate
        // formatting element copied by the inner loop, if one exists;
        // otherwise it remains at the original formatting element position.
        int current_format=active_index(formatting);
        if(current_format<0){error_=BuildError::FormattingLimit;return false;}
        size_t bookmark=static_cast<size_t>(current_format);
        if(bookmark_after!=NO){
            const int index=active_index(bookmark_after);
            if(index<0){error_=BuildError::FormattingLimit;return false;}
            bookmark=static_cast<size_t>(index)+1;
        }
        active_remove_at(static_cast<size_t>(current_format));
        if(bookmark>static_cast<size_t>(current_format))--bookmark;
        if(!active_insert(bookmark,replacement))return false;

        // Drop the old formatter; insert the new formatter directly after
        // furthestBlock in the open-elements stack. Retain all intervening
        // nodes not explicitly removed by the algorithm.
        stack_remove(format_pos);
        size_t updated_furthest=0;
        if(!on_open_stack(furthest,&updated_furthest)){
            error_=BuildError::TokenOverflow;return false;
        }
        if(!stack_insert(updated_furthest+1,replacement))return false;
        // The HTML algorithm can restart up to eight times for one end tag.
        // Normally the next iteration finds no further special block and
        // removes the replacement from the active list and open stack.
    }
    return true;
}

uint16_t TreeBuilder::create(NodeKind kind, uint16_t parent, const char* name,
                             const Token* token, bool implied, uint16_t before) noexcept {
    if (count_ >= node_limit() || parent >= count_) {
        error_ = BuildError::NodeLimit; return NO;
    }
    if (before!=NO && (before>=count_ || nodes_[before].parent!=parent)) {
        error_ = BuildError::TokenOverflow; return NO;
    }
    if (token && token->attr_count > 6) {
        error_ = BuildError::AttributeLimit; return NO;
    }
    const uint16_t predecessor=previous_sibling(parent,before);
    uint16_t index=NO;
    if(!allocate_node(index))return NO;
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
bool TreeBuilder::table_text_context() const noexcept {
    if(depth_<=1)return false;
    const char* current=nodes_[open_[depth_-1]].name;
    return equal(current,"table") || equal(current,"tbody") ||
           equal(current,"thead") || equal(current,"tfoot") ||
           equal(current,"tr");
}
bool TreeBuilder::collect_table_text(const Token& t) noexcept {
    pending_table_active_=true;
    // NUL tokens are parse errors and ignored in the HTML in-table-text mode.
    // Tokens can be split at the H1 tokenizer's 1024-byte output bound.
    for(size_t i=0;i<t.text_len;++i) {
        const char c=t.text[i];
        if(c=='\0')continue;
        if(pending_table_len_>=kPendingTableTextLimit){
            error_=BuildError::TokenOverflow;
            return false;
        }
        pending_table_text_[pending_table_len_++]=c;
    }
    return true;
}
bool TreeBuilder::flush_table_text() noexcept {
    if(!pending_table_active_)return true;
    pending_table_active_=false;
    const size_t n=pending_table_len_;
    pending_table_len_=0;
    if(!n)return true;
    const bool mixed=!whitespace_only(pending_table_text_,n);
    uint16_t parent=open_[depth_-1],before=NO;
    if(mixed){
        // Reprocess the *entire* pending run through in-body with foster
        // parenting. This preserves spaces at the beginning of a later
        // non-whitespace run, unlike flushing each Tokenizer chunk alone.
        if(!reconstruct_formatting())return false;
        parent=open_[depth_-1];
        foster_location(parent,before);
    }
    return append_text(parent,pending_table_text_,n,before);
}
bool TreeBuilder::table_open_in_scope() const noexcept {
    for(size_t i=depth_;i>1;--i) {
        const char* tag=nodes_[open_[i-1]].name;
        if(equal(tag,"table"))return true;
        if(equal(tag,"template"))return false;
    }
    return false;
}
bool TreeBuilder::close_caption_context() noexcept {
    // Scope-bound caption closing: do not pop beyond innermost table.
    for(size_t i=depth_;i>1;--i) {
        const char* tag=nodes_[open_[i-1]].name;
        if(equal(tag,"caption")){
            depth_=i-1;
            active_clear_marker();
            return true;
        }
        if(equal(tag,"table") || equal(tag,"template"))return false;
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
    const bool group = equal(name,"tbody") || equal(name,"thead") ||
                       equal(name,"tfoot") || equal(name,"caption") ||
                       equal(name,"colgroup");
    const bool row=equal(name,"tr");
    const bool cell=equal(name,"td") || equal(name,"th");
    if(!group && !row && !cell)return true;
    // All three table "clear the stack back to ... context" variants are
    // restricted to the *innermost* table. For example <table><b><tr>
    // must pop the misnested b before creating the implied tbody.
    size_t table_pos=depth_;
    for(size_t i=depth_;i>1;--i) {
        if(equal(nodes_[open_[i-1]].name,"table")){table_pos=i-1;break;}
        if(equal(nodes_[open_[i-1]].name,"template"))return true;
    }
    if(table_pos==depth_)return true;
    size_t group_pos=depth_,row_pos=depth_;
    for(size_t i=table_pos+1;i<depth_;++i) {
        const char* tag=nodes_[open_[i]].name;
        if(equal(tag,"tbody")||equal(tag,"thead")||equal(tag,"tfoot"))
            group_pos=i;
        if(equal(tag,"tr"))row_pos=i;
    }
    if(group) {
        // In-table insertion of a new row group/caption/colgroup clears
        // formatting and stale open cells back to the table itself.
        depth_=table_pos+1;
        return true;
    }
    // For a row, clear back to an existing row group (table-body mode)
    // or to the table (in-table mode, which implies tbody).
    if(row){
        depth_=(group_pos<depth_?group_pos:table_pos)+1;
        // group_pos is an index in the original stack; detect its presence
        // using the actual top of the now truncated stack.
        if(!equal(nodes_[open_[depth_-1]].name,"tbody") &&
           !equal(nodes_[open_[depth_-1]].name,"thead") &&
           !equal(nodes_[open_[depth_-1]].name,"tfoot")){
            const uint16_t id=create(NodeKind::Element,open_[depth_-1],
                                     "tbody",nullptr,true);
            if(!push(id))return false;
        }
        return true;
    }
    // For a cell, clear to the current row. If there is no row, HTML
    // constructs an implied row group and then an implied row.
    if(row_pos<depth_)depth_=row_pos+1;
    else if(group_pos<depth_)depth_=group_pos+1;
    else depth_=table_pos+1;
    if(!equal(nodes_[open_[depth_-1]].name,"tr")){
        if(!equal(nodes_[open_[depth_-1]].name,"tbody") &&
           !equal(nodes_[open_[depth_-1]].name,"thead") &&
           !equal(nodes_[open_[depth_-1]].name,"tfoot")){
            const uint16_t id=create(NodeKind::Element,open_[depth_-1],
                                     "tbody",nullptr,true);
            if(!push(id))return false;
        }
        const uint16_t id=create(NodeKind::Element,open_[depth_-1],
                                 "tr",nullptr,true);
        if(!push(id))return false;
    }
    return true;
}
bool TreeBuilder::start_body_tag(const Token& t) noexcept {
    const char* n=t.name;
    if(equal(n,"html")||equal(n,"body")||equal(n,"head")) return true;
    // A caption is a special table insertion mode. A new table structure
    // token first closes the caption in its own innermost table scope.
    if(equal(n,"caption") || equal(n,"colgroup") || equal(n,"col") ||
       equal(n,"tbody") || equal(n,"thead") || equal(n,"tfoot") ||
       equal(n,"tr") || equal(n,"td") || equal(n,"th")){
        close_caption_context();
    }
    if(equal(n,"a") && active_last("a")>=0) {
        // HTML does not permit nested active hyperlinks. Resolve the prior
        // active anchor before starting the next anchor.
        if(!adopt_formatting_end("a"))return false;
    }
    if(equal(n,"p"))close_open("p");
    else if(equal(n,"li"))close_list_item("li");
    else if(equal(n,"dt")||equal(n,"dd"))close_list_item(n);
    else if(is_heading(n))close_heading();
    else if(is_block(n))close_open("p");
    if(equal(n,"tr")) {
        if(close_open_in_table("td")||close_open_in_table("th"))active_clear_marker();
        close_open_in_table("tr");
    }
    if(equal(n,"td")||equal(n,"th")) {
        if(close_open_in_table("td")||close_open_in_table("th"))active_clear_marker();
    }
    if(!ensure_table_context(n))return false;
    if(equal(n,"col") && table_text_context() &&
       equal(nodes_[open_[depth_-1]].name,"table")){
        // The "in table" <col> rule implies a colgroup before reprocessing
        // the col token. The actual column element remains void.
        const uint16_t group=create(NodeKind::Element,open_[depth_-1],
                                    "colgroup",nullptr,true);
        if(!push(group))return false;
    }
    const bool table_structure =
        equal(n,"caption") || equal(n,"colgroup") || equal(n,"col") ||
        equal(n,"tbody") || equal(n,"thead") || equal(n,"tfoot") ||
        equal(n,"tr") || equal(n,"td") || equal(n,"th");
    if(!table_structure && !is_block(n) && !reconstruct_formatting())
        return false;
    uint16_t parent=open_[depth_-1], before=NO;
    if(!table_structure)foster_location(parent,before);
    const uint16_t id=create(NodeKind::Element,parent,n,&t,false,before);
    if(id==NO)return false;
    if(is_void(n))return true;
    if(!push(id))return false;
    if(is_formatting(n) && !active_add(id))return false;
    if(equal(n,"td")||equal(n,"th")||equal(n,"caption")){
        active_marker();
        if(error_!=BuildError::None)return false;
    }
    // The slash on a normal HTML (non-void) element is ignored.
    return true;
}
bool TreeBuilder::end_body_tag(const Token& t) noexcept {
    if(equal(t.name,"table"))close_caption_context();
    if(equal(t.name,"html")) { mode_=Mode::AfterBody; return true; }
    if(equal(t.name,"body")) { mode_=Mode::AfterBody;return true; }
    if(equal(t.name,"head"))return true;
    if(is_formatting(t.name))return adopt_formatting_end(t.name);
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
    if(equal(t.name,"tr")){
        if(close_open_in_table("td")||close_open_in_table("th"))active_clear_marker();
    }
    if(equal(t.name,"caption")) {
        close_caption_context();
        return true;
    }
    if(equal(t.name,"td")||equal(t.name,"th")){
        if(close_open_in_table(t.name))active_clear_marker();
        return true;
    }
    if(equal(t.name,"tr")||equal(t.name,"tbody")||
       equal(t.name,"thead")||equal(t.name,"tfoot")||
       equal(t.name,"colgroup"))
        close_open_in_table(t.name);
    else close_open(t.name);
    return true;
}
bool TreeBuilder::in_body(const Token& t) noexcept {
    if(t.kind==Kind::Text){
        // The "in table text" mode queues all adjacent character tokens,
        // even when H1 split them into bounded chunks. Mixed runs are then
        // foster-parented as a unit; all-whitespace runs remain in the table.
        if(pending_table_active_ || table_text_context())
            return collect_table_text(t);
        if(!whitespace_only(t.text,t.text_len) && !reconstruct_formatting())
            return false;
        return append_token_text(t);
    }
    if(!flush_table_text())return false;
    if(t.kind==Kind::Comment)return append_comment(t);
    if(t.kind==Kind::StartTag)return start_body_tag(t);
    if(t.kind==Kind::EndTag)return end_body_tag(t);
    return true;
}
bool TreeBuilder::handle(const Token& t) noexcept {
    if(t.overflow) { error_=BuildError::TokenOverflow;return false; }
    if(t.kind==Kind::Doctype){
        if(!flush_table_text())return false;
        const uint16_t id=create(NodeKind::Doctype,0,t.name,nullptr,false);
        return id!=NO;
    }
    if(t.kind==Kind::Comment) {
        if(!flush_table_text())return false;
        return append_comment(t);
    }
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
    if(error_!=BuildError::None)return false;
    if(!source && length){error_=BuildError::TokenOverflow;return false;}
    Tokenizer tok(source,length);
    for(size_t i=0;i<kMaxTokens;++i) {
        const Token t=tok.next();
        if(t.kind==Kind::Eof) {
            if(!flush_table_text())return false;
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
