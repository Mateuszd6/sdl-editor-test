#include <algorithm>
#include <string>

// We ignore all warnings, because parsing the headers of C libraries with
// Weverything causes a lot of them and they are meaningless.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"

// SDL2:
#include <SDL.h>

// Freetype2:
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#pragma clang diagnostic pop

// TODO(Cleanup): Try to use as less globals as possible...
namespace platform::global
{
    // TODO(Platform): Make this platform-dependent.
    static SDL_Window *window;

    // This is a surface that we put stuff on, and after that bit it to the
    // screen. This uses SDL software rendering and there is no use of
    // SLD_Renderer and stuff. After a resizing this pointer is changed.
    // Do not free it!
    // TODO(Platform): Make this platform-dependent.
    static SDL_Surface* screen;

    static glyph_data alphabet_[256];
    static glyph_data alphabet_colored_[256];

    static FT_Library library;
    static FT_Face face;

    static int32 line_height;
    static int32 font_ascent;
    static int32 font_descent;

    static int32 font_size = 14;
    static int32 linum_horizontal_offset = 20;
    static auto color_scheme = color_scheme_data{ // Monokai color theme.
        0x272822,
        0xF6F6F6,
        0xF92672,
        0x8f908a,
    };
}

// TODO: Move or get rid of!
namespace platform::temp
{
    static uint8* alphamap;
    static int32 alphamap_width;
    static int32 alphamap_height;

    static int32 texture_x_offset;
}

// TODO: Copied from SDL_TTF. These are used to calculate font metrics correctly.
#define FT_FLOOR(X) ((X & -64) / 64)
#define FT_CEIL(X)  (((X + 63) & -64) / 64)

namespace platform::detail
{
    static void set_letter_glyph(int16 letter)
    {
        // TODO(Cleanup): Add user control over whether or not he wants to autohint fonts!
        auto error = FT_Load_Char(global::face, letter, FT_LOAD_DEFAULT); // FT_LOAD_FORCE_AUTOHINT

        if(error)
            PANIC("Could not load char! %d", error);

        error = FT_Render_Glyph(global::face->glyph, FT_RENDER_MODE_NORMAL);
        if(error)
            PANIC("Could not render the glyph bitmap!");

        auto w = global::face->glyph->bitmap.width;
        auto h = global::face->glyph->bitmap.rows;

        // Save the metrics provided by Freetype2 to my metric system.
        // TODO: x_max, y_max!!!
        auto metrics = global::face->glyph->metrics;
        global::alphabet_[letter].metrics.x_min = static_cast<int>(metrics.horiBearingX) / 64;
        global::alphabet_[letter].metrics.y_min = -static_cast<int>(metrics.horiBearingY) / 64;
        global::alphabet_[letter].metrics.advance = static_cast<int>(metrics.horiAdvance / 64);
        global::alphabet_[letter].texture_x_offset = temp::texture_x_offset;
        global::alphabet_[letter].texture_y_offset = 1;
        global::alphabet_[letter].texture_width = w;
        global::alphabet_[letter].texture_height = h;

        auto bitmap_buffer = global::face->glyph->bitmap.buffer;
        auto dest_y_offset = 1;
        for(auto y = 0; y < static_cast<int32>(h); ++y)
        {
            auto src_ptr = bitmap_buffer + y * w;
            auto dest_ptr = temp::alphamap + temp::texture_x_offset +
                (dest_y_offset + y) * temp::alphamap_width;
            for(auto x = 0; x < static_cast<int32>(w); ++x)
                *dest_ptr++ = *src_ptr++;
        }

        temp::texture_x_offset += w + 1;
    }
}

namespace platform
{
    static void draw_rectangle_on_screen(::graphics::rectangle const& rect,
                                         ::graphics::color const& col)
    {
        auto sdl_rect = SDL_Rect { rect.x, rect.y, rect.width, rect.height };
        SDL_FillRect(global::screen, &sdl_rect, col.int_color);
    }

    static void initialize_font(char const* font_path)
    {
#if 0
        // Yeees, you should ask your graphics library for the resolution and
        // then pass it to FT, but FT defaults are be widely used and I don't
        // see any difference, so who cares?
        {
            real32 ddpi;
            real32 hdpi;
            real32 vdpi;
            auto dpi_request_error = SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi);
            if(dpi_request_error)
                PANIC("Could not get the scren dpi!");
        }
#endif

        // Init the library handle.
        {
            auto error = FT_Init_FreeType(&global::library);
            if(error)
                PANIC("Error initializing freetype. Game over... :(");
        }

        {
            auto error = FT_New_Face(global::library, font_path, 0, &global::face);

            if(error == FT_Err_Unknown_File_Format)
                PANIC("Font format is unsupported");
            else if(error)
                PANIC("Font file could not be opened or read, or is just broken");
        }

        // For now we do not support unscalable fonts.
        ASSERT(FT_IS_SCALABLE(global::face));

        {
            auto error = FT_Set_Char_Size(global::face,
                                          global::font_size * 64, 0,
                                          0, 0); // Default DPI.
            if(error)
                PANIC("Setting the size failed.");
        }

        auto scale = global::face->size->metrics.y_scale;
        global::font_ascent = FT_CEIL(FT_MulFix(global::face->ascender, scale));
        global::font_descent = FT_CEIL(FT_MulFix(global::face->descender, scale));
        global::line_height = FT_CEIL(FT_MulFix(global::face->height, scale));

        // TODO: Width is far too much. Height might be too small.
        temp::texture_x_offset = 0;

        // TODO: Width is far too much. Height might be too small.
        temp::alphamap_width = (global::line_height + 10) * 200;
        temp::alphamap_height = global::line_height + 10;
        temp::alphamap = static_cast<uint8*>(malloc(temp::alphamap_width * temp::alphamap_height));

        // For now we load only ASCII characters.
        for (auto c = ' '; c < 127; ++c)
            detail::set_letter_glyph(static_cast<int16>(c));
    }

    static void destroy_font()
    {
        // TODO: I dont want to free the font yet.
        return;
    }

    static void blit_letter(int16 character, uint32 color,
                            int32 X, int32 Y, int32* advance,
                            graphics::rectangle const* viewport_rect)
    {
        auto glyph = global::alphabet_[character];

        // If we are rendering 'starting from bottom' the last line might be
        // cutted from the top, so we make sure we always draw in correctly.
        auto sdl_rect = SDL_Rect {
            std::max(X + glyph.metrics.x_min, viewport_rect->x),
            std::max(Y + glyph.metrics.y_min, viewport_rect->y),
            0, 0,
        };

        if(sdl_rect.x < 0 || sdl_rect.y < 0)
        {
            LOG_ERROR("Glyph metrics seems incorrect. Letter %c will not be blited!",
                      static_cast<char>(character));

            return;
        }

        // If advance was requested, we fill the pointer this with information.
        if(advance)
            *advance = glyph.metrics.advance;

        // TODO: Calculations are horriebly horrible but they do work at least.
        auto letter_rect = SDL_Rect{
            glyph.texture_x_offset,
            glyph.texture_y_offset,
            glyph.texture_width,
            (Y + glyph.metrics.y_min + glyph.texture_height >
             viewport_rect->y + viewport_rect->height - glyph.texture_height
             ? viewport_rect->y + viewport_rect->height - (Y + glyph.metrics.y_min)
             : glyph.texture_height)
        };

        // TODO: Not sure what to do if it is not the for byte surface.
        ASSERT(global::screen->format->BytesPerPixel == 4);

        auto rmask = global::screen->format->Rmask;
        auto gmask = global::screen->format->Gmask;
        auto bmask = global::screen->format->Bmask;

        auto rshift = __builtin_ffsll(rmask / 0xFF) - 1;
        auto gshift = __builtin_ffsll(gmask / 0xFF) - 1;
        auto bshift = __builtin_ffsll(bmask / 0xFF) - 1;

#if 1
        if(sdl_rect.y >= global::screen->h || sdl_rect.x >= global::screen->w)
            LOG_ERROR("Blitting completly outside the surface! This should never happen.");
#endif

        // TODO: This really NEEDS a refactor.
        auto stop = false;
        auto pixels = static_cast<uint32*>(global::screen->pixels);
        for(int32 y = sdl_rect.y; y < sdl_rect.y + letter_rect.h && !stop; ++y)
            for(int32 x = sdl_rect.x; x < sdl_rect.x + letter_rect.w; ++x)
            {
                if(x >= global::screen->w)
                    continue;

                if(y >= global::screen->h)
                {
                    stop = true;
                    break;
                }

                auto dest = pixels + y * global::screen->pitch / 4 + x;

                auto aph_tex_x = letter_rect.x + x - sdl_rect.x;
                auto aph_tex_y = letter_rect.y + y - sdl_rect.y;

                auto alpha = *(temp::alphamap + (aph_tex_y * temp::alphamap_width) + aph_tex_x);
                auto alpha_real = static_cast<real32>(alpha) / 255.0f;

                auto dest_r = (::platform::get_curr_scheme().background & rmask) >> rshift;
                auto dest_g = (::platform::get_curr_scheme().background & gmask) >> gshift;
                auto dest_b = (::platform::get_curr_scheme().background & bmask) >> bshift;

                auto source_r = (color & rmask) >> rshift;
                auto source_g = (color & gmask) >> gshift;
                auto source_b = (color & bmask) >> bshift;

                auto res_r = (1.0f - alpha_real) * dest_r + alpha_real * source_r;
                auto res_g = (1.0f - alpha_real) * dest_g + alpha_real * source_g;
                auto res_b = (1.0f - alpha_real) * dest_b + alpha_real * source_b;

                auto res = ((static_cast<uint32>(res_r + 0.5f) << rshift) |
                            (static_cast<uint32>(res_g + 0.5f) << gshift) |
                            (static_cast<uint32>(res_b + 0.5f) << bshift));

                *dest = res;
            }
    }

    // TODO(Cleanup): It is not java, get rid of getters.
    static int32 get_letter_width()
    {
        return global::alphabet_[static_cast<int32>(' ')].metrics.advance;
    }

    static int32 get_line_height()
    {
        return global::line_height;
    }

    static int32 get_font_ascent()
    {
        return global::font_ascent;
    }

    static int32 get_font_descent()
    {
        return global::font_descent;
    }

    static int32 get_font_size()
    {
        return global::font_size;
    }

    static color_scheme_data get_curr_scheme()
    {
        return global::color_scheme;
    }

    // Remove cursor drawing from here!
    template<typename STR>
    static void print_text_line(editor::window const* window_ptr,
                                STR& text,
                                int color,
                                int64 line_nr, // First visible line of the buffer is 0.
                                int64 cursor_idx,
                                int32 x_offset,
                                int32 y_offset,
                                bool start_from_top)
    {
        auto X = static_cast<int32>(window_ptr->position.x + x_offset);
        auto Y = static_cast<int32>(window_ptr->position.y + y_offset +
                                    ::platform::get_line_height() * (line_nr + 1));

        // We will calculate it as we move character by character.
        auto cursor_x = X;
        auto cursor_y = Y;
        auto text_len = text.size();

        // TODO: Still wondering if the second condition is needed. What aboutY?
        for (auto i = 0_u64;
             i < text_len && X <= window_ptr->position.x + window_ptr->position.width;
             ++i)
        {
            auto text_idx = static_cast<int16>(text[i]);

            // TODO: This looks like a reasonable default, doesn't it?
            auto advance = ::platform::get_letter_width();
            ::platform::blit_letter(text_idx, color,
                                    X, Y, &advance,
                                    &window_ptr->position);

            X += advance;

            if(static_cast<uint64>(cursor_idx) == i + 1)
            {
                cursor_x = X;
                cursor_y = Y;
            }
        }

        // Draw a cursor:
        if (cursor_idx >= 0)
        {
            auto rect = graphics::rectangle {
                // TODO: Removing 1 from the caret x position makes it look
                // exacly like in Siblime, but without subpixler rendering wider
                // characters don't look great sometimes.
                static_cast<int32>(cursor_x - 1),
                static_cast<int32>(window_ptr->position.y + y_offset +
                                   get_line_height() * line_nr - get_font_descent()),
                1,
                get_line_height()
            };

            draw_rectangle_on_screen(rect, graphics::make_color(::platform::get_curr_scheme().foreground));
        }
    }

    // TODO(Cleanup): Check if something here can fail, if not change the type to
    // void.
    static int redraw_window()
    {
        global::screen = SDL_GetWindowSurface(::platform::global::window);
        if (!global::screen)
            PANIC("Couldnt get the right surface of the window!");

        SDL_GetWindowSize(::platform::global::window,
                          &::graphics::global::window_w,
                          &::graphics::global::window_h);

        // This is probobly obsolete and should be removed. This bug was fixed long
        // time ago. This used to happen when too many windows were created due to
        // the stack smashing.
        if (::graphics::global::window_w == 0 || ::graphics::global::window_h == 0)
            PANIC("Size is 0!");

        ASSERT(editor::global::number_of_windows > 0);
        auto main_window = (editor::global::windows_arr + 1);
        auto mini_buffer_window = (editor::global::windows_arr + 0);

        // TODO(Splitting lines): Decide how i want to draw them.
        graphics::DrawSplittingLine({
                0, ::graphics::global::window_h - 17,
                ::graphics::global::window_w, 1
            });

        mini_buffer_window->redraw(editor::global::current_window_idx == 0);
        main_window->redraw(editor::global::current_window_idx == 1);

        auto gap_w = editor::global::windows_arr + 1;
        auto b_point = &gap_w->buf_point;

        // TODO: There is still no space for the cutted line, and it can easilly be displayed here.
        auto number_of_displayed_lines = static_cast<uint64>(
            (gap_w->position.height - 2) / ::platform::get_line_height());

        if(b_point->curr_line < b_point->first_line)
        {
            b_point->starting_from_top = true;
            b_point->first_line = b_point->curr_line;
        }
        else if(b_point->curr_line >= b_point->first_line + number_of_displayed_lines)
        {
            b_point->starting_from_top = false;
            b_point->first_line = b_point->curr_line - number_of_displayed_lines + 1;
        }

        auto horizontal_offset = 0;
        auto vertical_offset = 0;
        auto linum_horizontal_offset = global::linum_horizontal_offset;
        auto max_linum_digits_visible = std::to_string(
            std::min(b_point->first_line + number_of_displayed_lines +
                     (b_point->starting_from_top ? 1 : 0),
                     b_point->buffer_ptr->size())).size();

        auto linum_rect = SDL_Rect{
            gap_w->position.x,
            gap_w->position.y,
            static_cast<int32>(max_linum_digits_visible) * get_letter_width() + 2 * linum_horizontal_offset,
            gap_w->position.height,
        };
        SDL_FillRect(global::screen, &linum_rect, ::platform::get_curr_scheme().background);

        auto lines_printed = 0_u64;
        auto first_line_offset = (b_point->starting_from_top
                                  ? 0
                                  : (gap_w->position.height - 2) -
                                  (::platform::get_line_height() * (number_of_displayed_lines + 1))
                                  + get_font_descent());

        for(auto i = b_point->first_line + (b_point->starting_from_top ? 0 : -1);
            i < b_point->buffer_ptr->size();
            ++i)
        {
            auto s = std::to_string(i + 1);
            print_text_line(gap_w,
                            s,
                            get_curr_scheme().linum,
                            lines_printed,
                            -1,
                            horizontal_offset + linum_horizontal_offset,
                            static_cast<int32>(first_line_offset) + vertical_offset,
                            b_point->starting_from_top);

            print_text_line(gap_w,
                            *b_point->buffer_ptr->get_line(i),
                            (b_point->buffer_ptr->get_line(i)->sso_enabled()
                             ? get_curr_scheme().keyword
                             : get_curr_scheme().foreground),
                            lines_printed,
                            i == b_point->curr_line ? b_point->curr_idx : -1,
                            horizontal_offset + linum_rect.x + linum_rect.w,
                            static_cast<int32>(first_line_offset) + vertical_offset,
                            b_point->starting_from_top);

            lines_printed++;
            // TODO: Check if this is correct in both cases.
            auto should_break = lines_printed >= number_of_displayed_lines + 1;
            if(should_break)
                break;
        }

        SDL_UpdateWindowSurface(::platform::global::window);
        return 0;
    }

}
