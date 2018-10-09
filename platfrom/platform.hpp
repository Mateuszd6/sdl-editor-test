#ifndef PLATFORM_PLATFORM_HPP_INCLUDED
#define PLATFORM_PLATFORM_HPP_INCLUDED

namespace platform
{
    static void draw_rectangle_on_screen(graphics::rectangle const& rect,
                                         graphics::color const& col);

    static void blit_letter(int16 character, int32 clip_height,
                            int32 X, int32 Y, int32* advance);

    static void blit_letter_colored(int16 character, int32 clip_height,
                                    int32 X, int32 Y, int32* advance);

    static int32 get_line_height();
    static int get_letter_height(); // TODO: Try to depracete these
    static int get_letter_width();

}

#endif // PLATFORM_PLATFORM_HPP_INCLUDED
