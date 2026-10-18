#pragma once
#include <stdint.h>
#include <stddef.h>

namespace trinity::css {
struct Rect { int32_t x{}, y{}, w{}, h{}; };
struct Style {
    // width: -1 = auto; otherwise fixed CSS pixels; percent_width = 10000 means 100%.
    int32_t width{-1};
    uint16_t percent_width{0};
    int32_t min_width{0};
    int32_t max_width{0};  // 0 = no maximum
    int32_t padding_left{0}, padding_right{0}, padding_top{0}, padding_bottom{0};
    int32_t border{0};
    int32_t margin_left{0}, margin_right{0}, margin_top{0}, margin_bottom{0};
    bool border_box{false};
};
struct LayoutBox {
    uint32_t node_id{0};
    Rect border{};
    Rect content{};
    bool overflow{false};
};

// First pass: static block/inline formatting only. A later retained DOM layout
// tree must own these records and make paint/hit-test consume the same coordinates.
class Flow {
public:
    static constexpr size_t kMaxDepth = 32;
    Flow(int32_t viewport_width, int32_t viewport_height) noexcept;
    bool open(uint32_t node_id, const Style& style, LayoutBox& out) noexcept;
    bool close(LayoutBox& out) noexcept;
    bool inline_item(int32_t width, int32_t height, Rect& out) noexcept;
    bool text_run(uint32_t glyph_count, int32_t glyph_width, int32_t line_height,
                  Rect& first_fragment, uint32_t& lines_used) noexcept;
    size_t depth() const noexcept { return depth_; }
private:
    struct Frame {
        LayoutBox box{};
        Style style{};
        int32_t cursor_y{0};
        int32_t cursor_x{0};
        int32_t line_height{0};
        bool is_root{false};
    };
    Frame frames_[kMaxDepth]{};
    size_t depth_{0};
    static int32_t clamp64(int64_t value) noexcept;
    void line_break(Frame& frame) noexcept;
};
}
