#pragma once
#include "trinity_html5_tokenizer.hpp"
#include <stddef.h>
#include <stdint.h>

namespace trinity::html5 {

enum class NodeKind : uint8_t { Document, Element, Text, Comment, Doctype };
enum class BuildError : uint8_t {
    None, NodeLimit, DepthLimit, AttributeLimit, TokenLimit, TokenOverflow, FormattingLimit
};

struct TreeAttribute {
    char name[48]{};
    char value[160]{};
    bool has_value{false};
};

// One immutable parsed-tree node. Sibling indexes share storage with elements,
// text, doctypes and comments. UINT16_MAX means "no node".
struct TreeNode {
    static constexpr uint16_t None = UINT16_MAX;
    NodeKind kind{NodeKind::Element};
    uint16_t parent{None}, first_child{None}, last_child{None}, next_sibling{None};
    char name[48]{};
    char text[256]{};
    uint16_t text_len{0};
    TreeAttribute attrs[6]{};
    uint8_t attr_count{0};
    bool implied{false};
};

// First bounded HTML5 tree-construction pass. This is NOT yet a complete
// WHATWG tree builder (bounded adoption-agency repair, partial foster parenting, templates,
// SVG/MathML namespaces or scripting-driven document.write).
//
// The large fixed node array belongs in an isolated service-owned allocation,
// not on Trinity's desktop stack or in kernel/static executable pages.
class TreeBuilder {
public:
    static constexpr uint16_t kMaxNodes = 384;
    static constexpr size_t kMaxDepth = 64;
    static constexpr size_t kMaxTokens = 16384;
    TreeBuilder() noexcept;
    bool build(const char* html, size_t bytes) noexcept;
    BuildError error() const noexcept { return error_; }
    size_t count() const noexcept { return count_; }
    const TreeNode* get(uint16_t index) const noexcept {
        return index < count_ ? &nodes_[index] : nullptr;
    }
    uint16_t html_index() const noexcept { return html_; }
    uint16_t head_index() const noexcept { return head_; }
    uint16_t body_index() const noexcept { return body_; }
    uint16_t first_named_child(uint16_t parent, const char* name) const noexcept;
private:
    enum class Mode : uint8_t { BeforeHtml, BeforeHead, InHead, AfterHead, InBody, AfterBody };
    TreeNode nodes_[kMaxNodes]{};
    uint16_t open_[kMaxDepth]{};
    size_t count_{1}, depth_{1};
    // A compact active-formatting list. The sentinel is also used as a marker,
    // bounding reconstruction to the current table cell or caption.
    static constexpr size_t kFormattingLimit = 64;
    uint16_t active_[kFormattingLimit]{};
    size_t active_count_{0};
    // WHATWG "in table text": buffer consecutive character tokens until
    // the next non-character token, then classify the entire run. The finite
    // cap is explicit: failure is preferable to misplacing text fragments.
    static constexpr size_t kPendingTableTextLimit = 8192;
    char pending_table_text_[kPendingTableTextLimit]{};
    size_t pending_table_len_{0};
    bool pending_table_active_{false};
    uint16_t html_{TreeNode::None};
    uint16_t head_{TreeNode::None};
    uint16_t body_{TreeNode::None};
    Mode mode_{Mode::BeforeHtml};
    BuildError error_{BuildError::None};

    static bool equal(const char* lhs, const char* rhs) noexcept;
    static bool whitespace_only(const char* text, size_t length) noexcept;
    static void copy(char* dst, size_t capacity, const char* src, bool& overflow) noexcept;
    static bool is_head_tag(const char* name) noexcept;
    static bool is_void(const char* name) noexcept;
    static bool is_block(const char* name) noexcept;
    static bool is_heading(const char* name) noexcept;
    static bool is_formatting(const char* name) noexcept;
    static bool is_special_block(const char* name) noexcept;
    uint16_t create(NodeKind kind, uint16_t parent, const char* name,
                    const Token* attrs, bool implied, uint16_t before = TreeNode::None) noexcept;
    bool push(uint16_t index) noexcept;
    bool active_add(uint16_t node) noexcept;
    void active_remove_at(size_t index) noexcept;
    void active_marker() noexcept;
    void active_clear_marker() noexcept;
    int active_last(const char* name) const noexcept;
    bool on_open_stack(uint16_t node, size_t* position = nullptr) const noexcept;
    bool reconstruct_formatting() noexcept;
    bool adopt_formatting_end(const char* name) noexcept;
    // Bounded active-list and stack edits for H3B2 adoption-agency repair.
    int active_index(uint16_t node) const noexcept;
    void stack_remove(size_t at) noexcept;
    bool stack_insert(size_t at, uint16_t node) noexcept;
    bool active_insert(size_t at, uint16_t node) noexcept;
    bool formatting_in_scope(size_t open_position) const noexcept;
    uint16_t clone_element(uint16_t source, uint16_t parent) noexcept;
    bool relocate(uint16_t node, uint16_t parent, uint16_t before) noexcept;
    bool move_children(uint16_t source, uint16_t target, uint16_t except) noexcept;
    bool enter_html(const Token* original) noexcept;
    bool enter_head(const Token* original) noexcept;
    bool finish_head() noexcept;
    bool enter_body(const Token* original) noexcept;
    bool append_text(uint16_t parent, const char* text, size_t length,
                     uint16_t before = TreeNode::None) noexcept;
    bool append_token_text(const Token& t) noexcept;
    bool table_text_context() const noexcept;
    bool collect_table_text(const Token& t) noexcept;
    bool flush_table_text() noexcept;
    bool close_caption_context() noexcept;
    bool table_open_in_scope() const noexcept;
    // HTML table foster parenting: adjusted insertion before an open table.
    // Only active when the insertion target is a table scaffold, never a cell.
    bool foster_location(uint16_t& parent, uint16_t& before) const noexcept;
    uint16_t previous_sibling(uint16_t parent, uint16_t before) const noexcept;
    bool handle(const Token& t) noexcept;
    bool in_body(const Token& t) noexcept;
    bool start_body_tag(const Token& t) noexcept;
    bool end_body_tag(const Token& t) noexcept;
    bool close_open(const char* name) noexcept;
    // Never close a cell/row owned by an outer nested table.
    bool close_open_in_table(const char* name) noexcept;
    bool close_heading() noexcept;
    bool close_list_item(const char* name) noexcept;
    bool ensure_table_context(const char* name) noexcept;
    bool append_comment(const Token& t) noexcept;
    void reset() noexcept;
};
}
