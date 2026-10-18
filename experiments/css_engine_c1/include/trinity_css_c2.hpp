#pragma once
#include "trinity_css_c1.hpp"
#include "trinity_html5_tree.hpp"
#include <stddef.h>
#include <stdint.h>

namespace trinity::css_c2 {

// C2 intentionally supports a bounded SELECTOR SUBSET: type, universal,
// #id, .class, [attribute], [attribute=value], descendants, children,
// comma-separated groups, and :root. Unsupported selectors fail closed.
enum class SelectorError : uint8_t {
    None, Syntax, Unsupported, GroupLimit, CompoundLimit,
    QualifierLimit, NameLimit
};
enum class Relation : uint8_t { None, Descendant, Child };
struct AttributeTest {
    char name[48]{}, value[96]{};
    bool has_value{false};
};
struct Compound {
    char tag[48]{}, id[64]{};
    char classes[4][64]{};
    AttributeTest attributes[4]{};
    uint8_t class_count{0},attribute_count{0};
    bool root{false},universal{false};
    Relation relation{Relation::None};
};
struct Specificity {
    uint16_t ids{0},classes{0},elements{0};
};
struct Selector {
    static constexpr size_t kMaxCompounds=8;
    Compound compounds[kMaxCompounds]{};
    uint8_t count{0};
    Specificity specificity{};
};
class SelectorList {
public:
    static constexpr size_t kMaxGroups=8;
    bool parse(const char* source,size_t bytes) noexcept;
    const Selector* get(size_t i) const noexcept {
        return i<count_?&selectors_[i]:nullptr;
    }
    size_t count() const noexcept{return count_;}
    SelectorError error() const noexcept{return error_;}
private:
    Selector selectors_[kMaxGroups]{};
    size_t count_{0};
    SelectorError error_{SelectorError::None};
};
bool matches(const Selector& selector,
             const trinity::html5::TreeBuilder& tree,
             uint16_t node) noexcept;
bool greater(Specificity lhs, Specificity rhs) noexcept;

// C2 computed-style subset. The immutable tree and pre-parsed C1 stylesheet
// are borrowed, never modified; CSS values remain bounded source strings.
// No CSSOM, dynamic pseudo-state, Flexbox/Grid, or production painting.
struct ComputedStyle {
    char color[96]{"black"};
    char background_color[96]{"transparent"};
    char display[48]{"inline"};
    char font_size[64]{"16px"};
    char font_weight[64]{"normal"};
    char margin[96]{"0"};
    char padding[96]{"0"};
};
enum class ResolveError : uint8_t {
    None, InvalidNode, InheritanceDepth, UnsupportedDocument
};
class StyleResolver {
public:
    // Caller owns a reusable selector scratch buffer. Never put this
    // potentially large SelectorList on a tiny Ring-3 GUI stack.
    StyleResolver(const trinity::html5::TreeBuilder& document,
                  const trinity::css_c1::Stylesheet& sheet,
                  SelectorList& scratch) noexcept
        : document_(document),sheet_(sheet),scratch_(scratch){}
    StyleResolver(const StyleResolver&)=delete;
    StyleResolver& operator=(const StyleResolver&)=delete;
    bool compute(uint16_t node,ComputedStyle& output) noexcept;
    ResolveError error() const noexcept{return error_;}
private:
    const trinity::html5::TreeBuilder& document_;
    const trinity::css_c1::Stylesheet& sheet_;
    SelectorList& scratch_;
    ResolveError error_{ResolveError::None};
};
} // namespace trinity::css_c2
