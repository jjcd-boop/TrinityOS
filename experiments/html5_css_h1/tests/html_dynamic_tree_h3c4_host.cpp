#include "trinity_html5_tree.hpp"
#include "trinity_html5_segmented_nodes.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cstddef>
#include <cstdint>

using namespace trinity::html5;
static int failures=0;
static constexpr uint16_t NO=TreeNode::None;
static void check(bool ok,const char* description){
    if(!ok){++failures;std::printf("FAIL: %s\n",description);}
}
struct Alloc {
    size_t requests{0}, frees{0},live{0};
    size_t allow{SIZE_MAX};
};
static void* a_alloc(size_t bytes,size_t alignment,void* context) noexcept {
    Alloc& a=*static_cast<Alloc*>(context);
    if(alignment>alignof(std::max_align_t) || a.requests>=a.allow)return nullptr;
    void* p=std::malloc(bytes);
    if(p){++a.requests; a.live+=bytes;}
    return p;
}
static void a_release(void* ptr,size_t bytes,size_t alignment,void* context) noexcept {
    (void)alignment;
    if(!ptr)return;
    Alloc& a=*static_cast<Alloc*>(context);
    ++a.frees;a.live-=bytes;std::free(ptr);
}
static NodeArenaMemory callbacks(Alloc& a) noexcept {
    return NodeArenaMemory{&a,a_alloc,a_release};
}
static std::string document(size_t elements){
    std::string html="<!doctype html><html><body>";
    for(size_t i=0;i<elements;++i)html+="<div>X</div>";
    html+="</body></html>";
    return html;
}
static void graph(const TreeBuilder& builder,size_t expected_elements){
    const uint16_t body=builder.body_index();
    check(body!=NO,"body exists");
    if(body==NO)return;
    uint16_t current=builder.get(body)->first_child;
    size_t nodes=0;
    while(current!=NO && nodes<expected_elements+2){
        const TreeNode* e=builder.get(current);
        check(e && std::strcmp(e->name,"div")==0,"sibling tag div");
        if(!e)break;
        check(e->parent==body,"sibling parent is body");
        const TreeNode* t=builder.get(e->first_child);
        check(t && t->kind==NodeKind::Text && t->text_len==1 &&
              t->text[0]=='X' && t->parent==current,"text within div");
        current=e->next_sibling;
        ++nodes;
    }
    check(nodes==expected_elements && current==NO,
          "every expected element and sibling is retained");
    check(builder.get(static_cast<uint16_t>(builder.count()))==nullptr,
          "out-of-bounds read rejected");
}
int main(){
    const size_t page=SegmentedNodeArena::segment_bytes();
    {
        static Alloc a{};
        static SegmentedNodeArena arena(callbacks(a),6000,page*750);
        static TreeBuilder parser(arena);
        check(parser.segmented_mode(),"explicit segmented builder");
        const auto html=document(1300);
        check(parser.build(html.data(),html.size()),"live HTML > 384 nodes parsed");
        check(parser.count()==2605,"doctype, skeleton and element/text node counts");
        check(arena.count()==parser.count(),"arena and parser node counts agree");
        check(arena.reserved_bytes()<=arena.byte_budget(),"actual memory within caller quota");
        graph(parser,1300);
        const auto small=document(2);
        check(parser.build(small.data(),small.size()),"rebuild releases old arena and succeeds");
        check(parser.count()==9 && arena.count()==9,"rebuilt tree has no stale nodes");
        graph(parser,2);
        check(a.live==arena.reserved_bytes(),"no leaked pages after rebuild");
        arena.clear();
        check(a.live==0,"arena clear releases all pages");
        // TreeBuilder retains an external arena and must not be used after
        // clearing it externally until build() resets and recreates root.
        check(parser.build("<p>again</p>",12),"fresh parse after external clear");
    }
    {
        static Alloc a{};
        static SegmentedNodeArena arena(callbacks(a),17,page*10);
        static TreeBuilder parser(arena);
        const auto html=document(100);
        check(!parser.build(html.data(),html.size()) &&
              parser.error()==BuildError::NodeLimit,
              "node-count quota propagates through real parser");
        check(parser.count()==17,"no nodes beyond node quota");
        check(arena.count()==17,"node quota leaves consistent allocation count");
        check(parser.build("<p>x</p>",8),"can reuse after quota failure");
    }
    {
        static Alloc a{};
        static SegmentedNodeArena arena(callbacks(a),100,page*2);
        static TreeBuilder parser(arena);
        const auto html=document(100);
        check(!parser.build(html.data(),html.size()) &&
              parser.error()==BuildError::MemoryBudget,
              "byte budget fails safely during real HTML construction");
        check(arena.count()<=16 && arena.reserved_bytes()==page*2,
              "budget never overcommitted");
        check(parser.build("<p>x</p>",8),"small tree after byte-budget failure");
    }
    {
        static Alloc a{};
        static SegmentedNodeArena arena(callbacks(a),100,page*10);
        static TreeBuilder parser(arena);
        a.allow=a.requests+1;
        const auto html=document(120);
        check(!parser.build(html.data(),html.size()) &&
              parser.error()==BuildError::AllocatorFailure,
              "Ring-3 allocator failure propagates");
        check(arena.count()<=8,"failed segment did not create bogus handles");
        a.allow=SIZE_MAX;
        check(parser.build("<div>recovered</div>",20),
              "allocator failure does not poison future document rebuild");
    }
    {
        static TreeBuilder legacy;
        check(!legacy.segmented_mode(),"legacy mode remains available");
        const auto oversized=document(300);
        check(!legacy.build(oversized.data(),oversized.size()) &&
              legacy.error()==BuildError::NodeLimit,
              "legacy fixed limit still rejects >384 nodes");
        const auto small=document(2);
        check(legacy.build(small.data(),small.size()),"legacy mode works after rejection");
        graph(legacy,2);
    }
    {
        // All old tests use the 384-node backend. Check that injecting a
        // segmented arena doesn't change DOM semantics for common HTML.
        static Alloc alloc{};
        static SegmentedNodeArena storage(callbacks(alloc),300,
                                           page*40);
        static TreeBuilder dynamic(storage);
        static TreeBuilder fixed;
        const char* cases[]={
            "<!doctype html><h1>hello</h1><p>world",
            "<div><b>one</b><i>two</i></div>",
            "<p><b>1<i>2</b>3</i>4",
            "<table><tr><td>one<td>two</table>",
            "<table><b><tr><td>A</td></tr>B</table>C",
            "<table><caption><b>Title</b><tr><td>X</td></tr></table>",
            "<table><tr><td><b>A<table><tr><td>B</b>C</td></tr></table></td></tr></table>",
            "<ul><li>one<li>two<li>three",
            "<script>x < y</script><p>text &amp; more</p>",
            "<div class=first class=second><img src=x>tail</div>"
        };
        for(const char* sample:cases){
            const size_t bytes=std::strlen(sample);
            const bool old_ok=fixed.build(sample,bytes);
            const bool new_ok=dynamic.build(sample,bytes);
            check(old_ok==new_ok && fixed.error()==dynamic.error(),
                  "legacy and segmented parse result parity");
            if(!old_ok || !new_ok)continue;
            check(fixed.count()==dynamic.count(),
                  "segmented and legacy node counts identical");
            if(fixed.count()!=dynamic.count())continue;
            for(size_t i=0;i<fixed.count();++i){
                const uint16_t id=static_cast<uint16_t>(i);
                const TreeNode *a=fixed.get(id), *b=dynamic.get(id);
                check(a && b && a->kind==b->kind && a->parent==b->parent &&
                      a->first_child==b->first_child &&
                      a->last_child==b->last_child &&
                      a->next_sibling==b->next_sibling &&
                      a->text_len==b->text_len && a->attr_count==b->attr_count &&
                      a->implied==b->implied &&
                      std::strcmp(a->name,b->name)==0 &&
                      std::strcmp(a->text,b->text)==0,
                      "segmented DOM preserves node structure and values");
                if(!a || !b)continue;
                for(uint8_t j=0;j<a->attr_count && j<b->attr_count;++j)
                    check(std::strcmp(a->attrs[j].name,b->attrs[j].name)==0 &&
                          std::strcmp(a->attrs[j].value,b->attrs[j].value)==0 &&
                          a->attrs[j].has_value==b->attrs[j].has_value,
                          "segmented DOM preserves attributes");
            }
        }
    }
    if(failures){std::printf("FAIL H3C4 live DOM segmented migration (%d)\n",failures);return 1;}
    std::puts("PASS H3C4 live TreeBuilder 2605 nodes, node/byte/OOM quotas, rebuild, legacy");
    return 0;
}
