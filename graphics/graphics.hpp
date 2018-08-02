#ifndef __GRAPHICS_GRAPHICS_HPP_INCLUDED__
#define __GRAPHICS_GRAPHICS_HPP_INCLUDED__

namespace graphics
{
    struct rectangle
    {
        int x;
        int y;
        int width;
        int height;
    };

    __attribute__ ((unused))
    static void DrawSplittingLine(rectangle const& rect);
}

#endif // __GRAPHICS_GRAPHICS_HPP_INCLUDED__
