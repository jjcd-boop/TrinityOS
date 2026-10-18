#include "trinity_css_flow.hpp"
#include <limits.h>

namespace trinity::css {
int32_t Flow::clamp64(int64_t x) noexcept {
    if (x < 0) return 0;
    if (x > INT32_MAX / 2) return INT32_MAX / 2;
    return static_cast<int32_t>(x);
}
Flow::Flow(int32_t width, int32_t height) noexcept {
    Frame& f = frames_[0]; depth_ = 1;
    f.box.node_id = 0; f.is_root = true;
    f.box.border = {0, 0, clamp64(width), clamp64(height)};
    f.box.content = f.box.border;
    f.cursor_x = 0; f.cursor_y = 0;
}
void Flow::line_break(Frame& f) noexcept {
    f.cursor_y = clamp64(static_cast<int64_t>(f.cursor_y) + f.line_height);
    f.cursor_x = f.box.content.x;
    f.line_height = 0;
}
bool Flow::open(uint32_t node_id, const Style& s, LayoutBox& out) noexcept {
    if (depth_ >= kMaxDepth) return false;
    Frame& parent = frames_[depth_ - 1];
    if (parent.line_height) line_break(parent);
    const int32_t available = clamp64(static_cast<int64_t>(parent.box.content.w)
         - s.margin_left - s.margin_right);
    const int32_t decorations = clamp64(static_cast<int64_t>(s.padding_left) +
         s.padding_right + static_cast<int64_t>(s.border) * 2);
    int32_t border_width = 0;
    if (s.width >= 0) {
        border_width = clamp64(static_cast<int64_t>(s.width) +
           (s.border_box ? 0 : decorations));
    } else if (s.percent_width) {
        const int32_t computed = clamp64(
            static_cast<int64_t>(parent.box.content.w) * s.percent_width / 10000);
        border_width = clamp64(static_cast<int64_t>(computed) +
            (s.border_box ? 0 : decorations));
    } else border_width = available;
    if (s.min_width > 0) border_width = border_width < s.min_width ? s.min_width : border_width;
    if (s.max_width > 0) border_width = border_width > s.max_width ? s.max_width : border_width;
    const int32_t content_width = clamp64(static_cast<int64_t>(border_width)-decorations);
    const int32_t outer_x = clamp64(static_cast<int64_t>(parent.box.content.x) + s.margin_left);
    const int32_t outer_y = clamp64(static_cast<int64_t>(parent.cursor_y) + s.margin_top);
    const int32_t content_x = clamp64(static_cast<int64_t>(outer_x) + s.border + s.padding_left);
    const int32_t content_y = clamp64(static_cast<int64_t>(outer_y) + s.border + s.padding_top);
    Frame& f = frames_[depth_++];
    f = {};
    f.box.node_id = node_id;
    f.box.border = {outer_x, outer_y, border_width, 0};
    f.box.content = {content_x, content_y, content_width, 0};
    f.box.overflow = border_width > available;
    f.style = s;
    f.cursor_x = content_x; f.cursor_y = content_y;
    out = f.box;
    return true;
}
bool Flow::close(LayoutBox& out) noexcept {
    if (depth_ <= 1) return false;
    Frame& f = frames_[depth_-1];
    if (f.line_height) line_break(f);
    const int32_t content_height = clamp64(static_cast<int64_t>(f.cursor_y) - f.box.content.y);
    f.box.content.h = content_height;
    f.box.border.h = clamp64(static_cast<int64_t>(content_height) +
        f.style.padding_top + f.style.padding_bottom + static_cast<int64_t>(f.style.border)*2);
    out = f.box;
    --depth_;
    Frame& parent = frames_[depth_ - 1];
    parent.cursor_y = clamp64(static_cast<int64_t>(f.box.border.y) + f.box.border.h +
                               f.style.margin_bottom);
    parent.cursor_x = parent.box.content.x;
    parent.line_height = 0;
    return true;
}
bool Flow::inline_item(int32_t width, int32_t height, Rect& out) noexcept {
    if (!depth_ || width < 0 || height < 0) return false;
    Frame& f=frames_[depth_-1];
    const int32_t w=clamp64(width), h=clamp64(height);
    if (f.cursor_x > f.box.content.x &&
        static_cast<int64_t>(f.cursor_x) + w >
        static_cast<int64_t>(f.box.content.x) + f.box.content.w) line_break(f);
    out = {f.cursor_x, f.cursor_y, w, h};
    f.cursor_x = clamp64(static_cast<int64_t>(f.cursor_x) + w);
    if (h > f.line_height) f.line_height = h;
    return true;
}
bool Flow::text_run(uint32_t glyph_count, int32_t glyph_width, int32_t line_height,
                    Rect& first_fragment, uint32_t& lines_used) noexcept {
    lines_used=0;first_fragment={};
    if (glyph_width <= 0 || line_height <= 0 || !depth_) return false;
    // Deterministic bounded fragmentation; layout still needs actual font
    // shaping and Unicode cluster measurement in a later pass.
    const uint32_t limit=65536;
    if (glyph_count>limit) return false;
    Frame& f=frames_[depth_-1];
    if (f.box.content.w < glyph_width) return false;
    uint32_t remaining=glyph_count;
    bool first=true;
    if (!remaining) return true;
    while (remaining) {
        int32_t room=clamp64(static_cast<int64_t>(f.box.content.x) +
            f.box.content.w - f.cursor_x);
        uint32_t count=static_cast<uint32_t>(room / glyph_width);
        if (count==0) { line_break(f); continue; }
        if (count>remaining) count=remaining;
        Rect fragment{};
        if (!inline_item(clamp64(static_cast<int64_t>(count)*glyph_width),
                         line_height, fragment)) return false;
        if (first) { first_fragment=fragment;first=false; }
        remaining -= count;
        ++lines_used;
        if (remaining) line_break(f);
    }
    return true;
}
} // namespace trinity::css
