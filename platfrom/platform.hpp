#ifndef __PLATFORM_PLATFORM_HPP_INCLUDED__
#define __PLATFORM_PLATFORM_HPP_INCLUDED__

namespace platform
{
    static void draw_rectangle_on_screen(graphics::rectangle const& rect,
                                         graphics::color const& col);

    static void blit_letter(int character, int clip_height, graphics::rectangle const& rect);
    static void blit_letter_colored(int character, int clip_height, graphics::rectangle const& rect);

    static int get_letter_height();
    static int get_letter_width();
}

#endif // __PLATFORM_PLATFORM_HPP_INCLUDED__
