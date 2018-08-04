#ifndef __GRAPHICS_GRAPHICS_HPP_INCLUDED__
#define __GRAPHICS_GRAPHICS_HPP_INCLUDED__

#include "../editor/window.hpp"

namespace graphics::global
{
    static auto font_size = 13.f;
    static int window_w, window_h;
}

namespace graphics
{
    struct rectangle
    {
        int32 x;
        int32 y;
        int32 width;
        int32 height;
    };

    struct color
    {
        union
        {
            uint32 int_color;
            struct
            {
                uint8 r;
                uint8 g;
                uint8 b;
                uint8 a;
            };
        };
    };

    static color make_color(uint32 value);

    static void DrawSplittingLine(rectangle const& rect);

#if 0
    static void print_text_line(editor::window const* window_ptr,
                                int line_nr, // First visible line of the buffer is 0.
                                char const* text,
                                int cursor_idx); // gap_buffer const* line
#endif
}

#endif // __GRAPHICS_GRAPHICS_HPP_INCLUDED__
