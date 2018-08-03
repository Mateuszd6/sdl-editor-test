#ifndef __GRAPHICS_GRAPHICS_HPP_INCLUDED__
#define __GRAPHICS_GRAPHICS_HPP_INCLUDED__

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
}

#endif // __GRAPHICS_GRAPHICS_HPP_INCLUDED__
