#ifndef PLATFORM_PLATFORM_HPP_INCLUDED
#define PLATFORM_PLATFORM_HPP_INCLUDED

namespace platform::global
{
    static auto font_size = 14;
    static auto background_hex_color = 0xF6F6F6; // 0x161616; // 0x272822
    static auto foreground_hex_color = 0x060606; // 0xFFFFFF
}

namespace platform
{
    static void draw_rectangle_on_screen(graphics::rectangle const& rect,
                                         graphics::color const& col);

    static void blit_letter(int16 character, int32 clip_height,
                            int32 X, int32 Y, int32* advance,
                            graphics::rectangle const* viewport_rect);

    static void blit_letter_colored(int16 character, int32 clip_height,
                                    int32 X, int32 Y, int32* advance,
                                    graphics::rectangle const* viewport_rect);

    static int32 get_line_height();
    static int get_letter_width();

    static int32 get_font_ascent();
    static int32 get_font_descent();
}

#endif // PLATFORM_PLATFORM_HPP_INCLUDED
