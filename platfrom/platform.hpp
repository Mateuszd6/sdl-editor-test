#ifndef PLATFORM_PLATFORM_HPP_INCLUDED
#define PLATFORM_PLATFORM_HPP_INCLUDED


namespace platform
{
    struct glyph_metrics
    {
        int32 x_min;
        int32 y_min;

        int32 advance;

        // NOTE: We dont keep x_max/width/height/etc... becauce we keep them as
        // texture_width/height in glyph_datta which contains this struct. Not a
        // super-design, but glyph_metrics values are explicitly from calling a
        // FT metrics functions, and other values in glyph_data are from the
        // measuring the texture, which(FIXME) might differ due to the AA.
    };


    struct glyph_data
    {
        int32 texture_width;
        int32 texture_height;
        int32 texture_pitch;
        glyph_metrics metrics;

        // TODO: Use one buffer that contains all info about fonts(or at least
        // just one face).
        uint8* texture_buffer;
    };

    struct color_scheme_data
    {
        uint32 background;
        uint32 foreground;
        uint32 keyword;
        uint32 linum;
    };

    static void draw_rectangle_on_screen(graphics::rectangle const& rect,
                                         graphics::color const& col);

    static void blit_letter(int16 character, uint32 color,
                            int32 X, int32 Y, int32* advance,
                            graphics::rectangle const* viewport_rect);

    static int32 get_line_height();
    static int32 get_letter_width();

    static int32 get_font_ascent();
    static int32 get_font_descent();

    static int32 get_font_size();
    static color_scheme_data get_curr_scheme();
}

#endif // PLATFORM_PLATFORM_HPP_INCLUDED
