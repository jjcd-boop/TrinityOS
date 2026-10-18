#include "trinity_css_c2.hpp"
namespace trinity::css_c2 {
namespace {
using trinity::html5::TreeBuilder;
using trinity::html5::TreeNode;
struct Scanner {
    const char* s;size_t n,pos{0};
    bool ws() const noexcept{
        return pos<n && (s[pos]==' '||s[pos]=='\n'||s[pos]=='\r'||
                         s[pos]=='\t'||s[pos]=='\f');
    }
    bool skip() noexcept{
        const size_t start=pos;while(ws())++pos;return pos!=start;
    }
};
bool ident_char(unsigned char c) noexcept {
    return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||
        (c>='0'&&c<='9')||c=='-'||c=='_'||c>=128;
}
bool identifier(Scanner& p,char* out,size_t cap) noexcept {
    const size_t start=p.pos;
    size_t length=0;
    while(p.pos<p.n && ident_char(static_cast<unsigned char>(p.s[p.pos]))){
        if(length+1>=cap)return false;
        out[length++]=p.s[p.pos++];
    }
    if(start==p.pos)return false;
    out[length]=0;return true;
}
bool keyword(const char* a,const char* b) noexcept {
    if(!a||!b)return false;
    for(;*a&&*b;++a,++b){
        char x=*a,y=*b;
        if(x>='A'&&x<='Z')x=static_cast<char>(x+32);
        if(y>='A'&&y<='Z')y=static_cast<char>(y+32);
        if(x!=y)return false;
    }
    return !*a&&!*b;
}
bool exact(const char* a,const char* b) noexcept {
    if(!a||!b)return false;
    while(*a&&*b&&*a==*b){++a;++b;}
    return *a==0&&*b==0;
}
bool in_classes(const char* all,const char* wanted) noexcept {
    if(!all||!wanted)return false;
    for(size_t i=0;all[i];){
        while(all[i]==' '||all[i]=='\t'||all[i]=='\n'||
              all[i]=='\r'||all[i]=='\f')++i;
        if(!all[i])break;
        size_t j=0;
        while(wanted[j] && all[i+j]==wanted[j])++j;
        if(!wanted[j] && (!all[i+j] || all[i+j]==' '||all[i+j]=='\t'||
                          all[i+j]=='\n'||all[i+j]=='\r'||all[i+j]=='\f'))
            return true;
        while(all[i] && all[i]!=' ' && all[i]!='\t'&&all[i]!='\n'&&
              all[i]!='\r'&&all[i]!='\f')++i;
    }
    return false;
}
const char* attr(const TreeNode& node,const char* name) noexcept {
    for(uint8_t i=0;i<node.attr_count;++i){
        if(keyword(node.attrs[i].name,name))return node.attrs[i].value;
    }
    return nullptr;
}
bool simple(const Compound& compound,const TreeBuilder& tree,uint16_t node) noexcept {
    const TreeNode* n=tree.get(node);
    if(!n||n->kind!=trinity::html5::NodeKind::Element)return false;
    if(compound.tag[0]&&!keyword(compound.tag,n->name))return false;
    if(compound.id[0] && !exact(attr(*n,"id"),compound.id))return false;
    for(uint8_t i=0;i<compound.class_count;++i)
        if(!in_classes(attr(*n,"class"),compound.classes[i]))return false;
    for(uint8_t i=0;i<compound.attribute_count;++i){
        const AttributeTest& a=compound.attributes[i];
        const char* value=attr(*n,a.name);
        if(!value || (a.has_value && !exact(value,a.value)))return false;
    }
    if(compound.root){
        const TreeNode* parent=tree.get(n->parent);
        if(!parent || parent->kind!=trinity::html5::NodeKind::Document)
            return false;
    }
    return true;
}
bool parse_attribute(Scanner& s,Compound& part) noexcept {
    if(part.attribute_count>=4)return false;
    ++s.pos;s.skip();
    AttributeTest& a=part.attributes[part.attribute_count];
    if(!identifier(s,a.name,sizeof(a.name)))return false;
    s.skip();
    if(s.pos<s.n && s.s[s.pos]=='='){
        ++s.pos;s.skip();a.has_value=true;
        if(s.pos<s.n && (s.s[s.pos]=='"'||s.s[s.pos]=='\'')){
            const char quote=s.s[s.pos++];size_t v=0;
            while(s.pos<s.n && s.s[s.pos]!=quote){
                if(v+1>=sizeof(a.value))return false;
                if(s.s[s.pos]=='\\')return false;
                a.value[v++]=s.s[s.pos++];
            }
            if(s.pos>=s.n)return false;
            a.value[v]=0;++s.pos;
        }else if(!identifier(s,a.value,sizeof(a.value)))return false;
        s.skip();
    }
    if(s.pos>=s.n || s.s[s.pos]!=']')return false;
    ++s.pos;++part.attribute_count;return true;
}
bool parse_compound(Scanner& s,Selector& selector,Relation relation,
                    SelectorError& err) noexcept {
    if(selector.count>=Selector::kMaxCompounds){
        err=SelectorError::CompoundLimit;return false;
    }
    Compound part{};part.relation=relation;
    bool any=false;
    if(s.pos<s.n && s.s[s.pos]=='*'){
        ++s.pos;part.universal=true;any=true;
    }else if(s.pos<s.n && s.s[s.pos]!='#'&&s.s[s.pos]!='.' &&
             s.s[s.pos]!='['&&s.s[s.pos]!=':'){
        if(!identifier(s,part.tag,sizeof(part.tag))){
            err=SelectorError::NameLimit;return false;
        }
        ++selector.specificity.elements;any=true;
    }
    while(s.pos<s.n) {
        const char c=s.s[s.pos];
        if(c=='#'){
            if(part.id[0]){err=SelectorError::Unsupported;return false;}
            ++s.pos;
            if(!identifier(s,part.id,sizeof(part.id))){
                err=SelectorError::Syntax;return false;
            }
            ++selector.specificity.ids;any=true;
        }else if(c=='.'){
            if(part.class_count>=4){err=SelectorError::QualifierLimit;return false;}
            ++s.pos;
            if(!identifier(s,part.classes[part.class_count],64)){
                err=SelectorError::Syntax;return false;
            }
            ++part.class_count;++selector.specificity.classes;any=true;
        }else if(c=='['){
            if(!parse_attribute(s,part)){
                err=SelectorError::Unsupported;return false;
            }
            ++selector.specificity.classes;any=true;
        }else if(c==':'){
            ++s.pos;
            char pseudo[48]{};
            if(!identifier(s,pseudo,sizeof(pseudo)) ||
               !keyword(pseudo,"root") || part.root){
                err=SelectorError::Unsupported;return false;
            }
            part.root=true;++selector.specificity.classes;any=true;
        }else break;
    }
    if(!any){err=SelectorError::Syntax;return false;}
    selector.compounds[selector.count++]=part;
    return true;
}
bool block_element(const char* name) noexcept {
    const char* tags[]={"html","body","div","p","h1","h2","h3","h4","h5","h6",
        "ul","ol","li","section","article","header","footer","main","aside",
        "nav","table","tr","tbody","thead","tfoot","blockquote","form",
        "address","figure","figcaption","pre","dl","dt","dd"};
    for(const char* tag:tags)if(keyword(name,tag))return true;
    return false;
}
bool assign(char* dest,size_t capacity,const char* src) noexcept {
    size_t n=0;if(!src)return false;
    while(src[n]){
        if(n+1>=capacity)return false;
        ++n;
    }
    for(size_t i=0;i<n;++i)dest[i]=src[i];
    dest[n]=0;return true;
}
struct Winner {
    bool set{false},important{false};
    Specificity score{};
    unsigned order{0};
};
bool is_better(const Winner& old,bool important,Specificity score,
               unsigned order) noexcept {
    if(!old.set)return true;
    if(old.important!=important)return important;
    if(greater(score,old.score))return true;
    if(greater(old.score,score))return false;
    return order>=old.order;
}
enum Property : uint8_t {
    Color, Background, Display, FontSize, FontWeight, Margin, Padding, Total
};
int which(const char* name) noexcept {
    if(keyword(name,"color"))return Color;
    if(keyword(name,"background-color"))return Background;
    if(keyword(name,"display"))return Display;
    if(keyword(name,"font-size"))return FontSize;
    if(keyword(name,"font-weight"))return FontWeight;
    if(keyword(name,"margin"))return Margin;
    if(keyword(name,"padding"))return Padding;
    return -1;
}
char* property_slot(ComputedStyle& style,int id) noexcept {
    switch(id){
        case Color:return style.color;
        case Background:return style.background_color;
        case Display:return style.display;
        case FontSize:return style.font_size;
        case FontWeight:return style.font_weight;
        case Margin:return style.margin;
        case Padding:return style.padding;
        default:return nullptr;
    }
}
size_t slot_size(int id) noexcept {
    switch(id){
        case Color:return sizeof(ComputedStyle::color);
        case Background:return sizeof(ComputedStyle::background_color);
        case Display:return sizeof(ComputedStyle::display);
        case FontSize:return sizeof(ComputedStyle::font_size);
        case FontWeight:return sizeof(ComputedStyle::font_weight);
        case Margin:return sizeof(ComputedStyle::margin);
        case Padding:return sizeof(ComputedStyle::padding);
        default:return 0;
    }
}
} // namespace

bool SelectorList::parse(const char* source,size_t bytes) noexcept {
    count_=0;error_=SelectorError::None;
    if(!source || !bytes){error_=SelectorError::Syntax;return false;}
    Scanner scan{source,bytes};scan.skip();
    while(scan.pos<scan.n){
        if(count_>=kMaxGroups){error_=SelectorError::GroupLimit;return false;}
        Selector& selector=selectors_[count_];selector=Selector{};
        Relation relation=Relation::None;
        for(;;){
            if(!parse_compound(scan,selector,relation,error_))return false;
            const bool spaced=scan.skip();
            if(scan.pos>=scan.n || scan.s[scan.pos]==',')break;
            if(scan.s[scan.pos]=='>'){
                ++scan.pos;scan.skip();relation=Relation::Child;
                if(scan.pos>=scan.n||scan.s[scan.pos]==','||
                   scan.s[scan.pos]=='>'){error_=SelectorError::Syntax;return false;}
            } else if(spaced){
                relation=Relation::Descendant;
            } else {
                error_=SelectorError::Unsupported;return false;
            }
        }
        ++count_;
        if(scan.pos>=scan.n)break;
        ++scan.pos;scan.skip();
        if(scan.pos>=scan.n){error_=SelectorError::Syntax;return false;}
    }
    return count_>0;
}
bool greater(Specificity a,Specificity b) noexcept {
    if(a.ids!=b.ids)return a.ids>b.ids;
    if(a.classes!=b.classes)return a.classes>b.classes;
    return a.elements>b.elements;
}
bool matches(const Selector& selector,const TreeBuilder& tree,
             uint16_t node) noexcept {
    if(!selector.count || !simple(selector.compounds[selector.count-1],tree,node))
        return false;
    uint16_t current=node;
    for(size_t i=selector.count-1;i>0;--i){
        const Compound& curr=selector.compounds[i];
        const Compound& prev=selector.compounds[i-1];
        const TreeNode* n=tree.get(current);
        if(!n)return false;
        uint16_t parent=n->parent;
        if(curr.relation==Relation::Child){
            if(!simple(prev,tree,parent))return false;
            current=parent;
        }else if(curr.relation==Relation::Descendant){
            bool found=false;
            size_t guard=0;
            while(parent!=TreeNode::None && guard++<TreeBuilder::kMaxDepth){
                if(simple(prev,tree,parent)){found=true;current=parent;break;}
                const TreeNode* a=tree.get(parent);
                if(!a)break;
                parent=a->parent;
            }
            if(!found)return false;
        }else return false;
    }
    return true;
}

bool StyleResolver::compute(uint16_t node,ComputedStyle& out) noexcept {
    error_=ResolveError::None;
    const TreeNode* requested=document_.get(node);
    if(!requested||requested->kind!=trinity::html5::NodeKind::Element){
        error_=ResolveError::InvalidNode;return false;
    }
    uint16_t ancestry[TreeBuilder::kMaxDepth]{};
    size_t depth=0;uint16_t cursor=node;
    while(cursor!=TreeNode::None){
        const TreeNode* current=document_.get(cursor);
        if(!current){error_=ResolveError::InvalidNode;return false;}
        if(current->kind==trinity::html5::NodeKind::Element){
            if(depth>=TreeBuilder::kMaxDepth){
                error_=ResolveError::InheritanceDepth;return false;
            }
            ancestry[depth++]=cursor;
        }
        cursor=current->parent;
    }
    ComputedStyle parent{};
    for(size_t level=depth;level>0;--level){
        const uint16_t target=ancestry[level-1];
        const TreeNode* element=document_.get(target);
        ComputedStyle result{};
        // Inheritance applies to selected typography properties only.
        if(!assign(result.color,sizeof(result.color),parent.color) ||
           !assign(result.font_size,sizeof(result.font_size),parent.font_size) ||
           !assign(result.font_weight,sizeof(result.font_weight),parent.font_weight)){
            error_=ResolveError::UnsupportedDocument;return false;
        }
        if(block_element(element->name))
            assign(result.display,sizeof(result.display),"block");
        Winner winners[Total]{};
        // C2 intentionally excludes unsupported pseudo selectors and
        // @media rules; fail closed rather than overapplying uncertain CSS.
        for(size_t ri=0;ri<sheet_.count();++ri){
            const trinity::css_c1::Rule* rule=sheet_.get(ri);
            if(!rule)continue;
            SelectorList& list=scratch_;
            size_t size=0;while(rule->selector[size])++size;
            if(!list.parse(rule->selector,size))continue;
            bool matched=false;Specificity best{};
            for(size_t si=0;si<list.count();++si){
                const Selector* sel=list.get(si);
                if(sel && matches(*sel,document_,target)){
                    if(!matched || greater(sel->specificity,best))
                        best=sel->specificity;
                    matched=true;
                }
            }
            if(!matched)continue;
            for(size_t di=0;di<rule->declaration_count;++di){
                const auto& declaration=rule->declarations[di];
                const int slot=which(declaration.property);
                if(slot<0)continue;
                const unsigned order=static_cast<unsigned>(ri*24+di);
                Winner& winner=winners[slot];
                if(!is_better(winner,declaration.important,best,order))
                    continue;
                char* dest=property_slot(result,slot);
                if(!dest || !assign(dest,slot_size(slot),declaration.value))
                    continue;
                winner={true,declaration.important,best,order};
            }
        }
        parent=result;
    }
    out=parent;return true;
}
} // namespace trinity::css_c2
