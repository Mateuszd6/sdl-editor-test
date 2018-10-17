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
    struct glyph_metrics
    {
        int32 x_min;
        int32 y_min;
#if 0
        int32 x_max;
        int32 y_max;
#endif
        int32 advance;

#if 0
        int32 width() { return x_max - x_min; }
        int32 height() { return y_max - y_min; }
#endif
    };

    struct glyph_data
    {
        int32 texture_x_offset;
        int32 texture_y_offset;
        int32 texture_width;
        int32 texture_height;
        glyph_metrics metrics;
    };

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
}

// TODO: Fix the metrics! This is how SDL_TTF handled this.
#if 0
    scale = face->size->metrics.y_scale;
    font->ascent  = FT_CEIL(FT_MulFix(face->ascender, scale));
    font->descent = FT_CEIL(FT_MulFix(face->descender, scale));
    font->height  = font->ascent - font->descent + /* baseline */ 1;
    font->lineskip = FT_CEIL(FT_MulFix(face->height, scale));
    font->underline_offset = FT_FLOOR(FT_MulFix(face->underline_position, scale));
    font->underline_height = FT_FLOOR(FT_MulFix(face->underline_thickness, scale))
#endif


// TODO: Move or get rid of!
namespace platform::temp
{
    static SDL_Surface* alphabet_texture;
    static int32 texture_x_offset;
}

// TODO: Copied from SDL_TTF. These are used to calculate font metrics correctly.
#define FT_FLOOR(X) ((X & -64) / 64)
#define FT_CEIL(X)  (((X + 63) & -64) / 64)

namespace platform::detail
{
    // TODO: Make them global and static!!!
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    auto constexpr rmask = 0xff000000;
    auto constexpr gmask = 0x00ff0000;
    auto constexpr bmask = 0x0000ff00;
    auto constexpr amask = 0x000000ff;
#else
    auto constexpr rmask = 0x000000ff;
    auto constexpr gmask = 0x0000ff00;
    auto constexpr bmask = 0x00ff0000;
    auto constexpr amask = 0xff000000;
#endif

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

        auto bitmap = global::face->glyph->bitmap.buffer;

        auto surface = SDL_CreateRGBSurface(0, w, h, 32,
                                            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        if (surface == nullptr)
        {
            PANIC("SDL_CreateRGBSurface() failed: %s", SDL_GetError());
            exit(1);
        }
        SDL_FillRect(surface, nullptr, global::foreground_hex_color);

        auto pixels = static_cast<uint8*>(surface->pixels);
        for(auto x = 0_u32; x < w; ++x)
            for(auto y = 0_u32; y < h; ++y)
            {
                auto alpha = bitmap[y * w + x];
                auto pixel_ptr = reinterpret_cast<int32*>(pixels + (4 * (y * w + x)));
                *pixel_ptr |= global::foreground_hex_color | (alpha << 24);
            }

        SDL_Rect source_rect{ 0, 0, static_cast<int32>(w), static_cast<int32>(h) };
        SDL_Rect dest_rect{ temp::texture_x_offset, 1, 0, 0 };
        SDL_BlitSurface(surface, &source_rect, temp::alphabet_texture, &dest_rect);

        temp::texture_x_offset += w + 1;
        SDL_FreeSurface(surface);
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
        // Init the library handle.
        {
            auto error = FT_Init_FreeType(&global::library);
            if(error)
                PANIC("Error initializing freetype. Game over... :(");
        }

        // Load the face from the file.
        {
            auto error = FT_New_Face(global::library, font_path, 0, &global::face);

            if(error == FT_Err_Unknown_File_Format)
                PANIC("Font format is unsupported");
            else if(error)
                PANIC("Font file could not be opened or read, or is just broken");
        }

        // Ask the graphics library for the monitors DPI.
        float ddpi;
        float hdpi;
        float vdpi;
        auto dpi_request_error = SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi);
        if(dpi_request_error != 0)
            PANIC("Could not get the scren dpi!");
        printf("dpi is: %f x %f\n", static_cast<double>(hdpi), static_cast<double>(vdpi));


        // For now we do not support unscalable fonts.
        ASSERT(FT_IS_SCALABLE(global::face));

        // This uses the default DPI (for now).
        auto error = FT_Set_Char_Size(global::face,
                                      global::font_size * 64, 0,
                                      0, 0);
        if(error)
            PANIC("Setting the error failed.");

        /* Get the scalable font metrics for this font */
        auto scale = global::face->size->metrics.y_scale;
        global::font_ascent = FT_CEIL(FT_MulFix(global::face->ascender, scale));
        global::font_descent = FT_CEIL(FT_MulFix(global::face->descender, scale));
        global::line_height = FT_CEIL(FT_MulFix(global::face->height, scale));

#if 0
        global::font->height  = global::font->ascent - global::font->descent + /* baseline */ 1;
        font->lineskip = FT_CEIL(FT_MulFix(face->height, scale));
        font->underline_offset = FT_FLOOR(FT_MulFix(face->underline_position, scale));
        font->underline_height = FT_FLOOR(FT_MulFix(face->underline_thickness, scale));
#endif

#if 0
        scale = face->size->metrics.y_scale;
        font->ascent  = FT_CEIL(FT_MulFix(face->ascender, scale));
        font->descent = FT_CEIL(FT_MulFix(face->descender, scale));
        font->height  = font->ascent - font->descent + /* baseline */ 1;
        font->lineskip = FT_CEIL(FT_MulFix(face->height, scale));
        font->underline_offset = FT_FLOOR(FT_MulFix(face->underline_position, scale));
        font->underline_height = FT_FLOOR(FT_MulFix(face->underline_thickness, scale))
#endif

#if 0
        global::line_height = static_cast<int32>(global::face->size->metrics.height) / 64;
        global::font_ascent = static_cast<int32>(global::face->size->metrics.ascender / 64);
        global::font_descent = static_cast<int32>(global::face->size->metrics.descender / 64);
#endif

        // TODO: Width is far too much. Height might be too small.
        temp::texture_x_offset = 0;
        temp::alphabet_texture = SDL_CreateRGBSurface(
            SDL_SWSURFACE,
            (global::line_height + 10) * 200,
            (global::line_height + 10),
            32,
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

        if (temp::alphabet_texture == nullptr)
        {
            PANIC("SDL_CreateRGBSurface() failed: %s", SDL_GetError());
            exit(1);
        }

        SDL_FillRect(temp::alphabet_texture, nullptr, global::foreground_hex_color);
    }

    static void destroy_font()
    {
        // TODO: I dont want to free the font yet.
        return;
    }

    static void run_font_test()
    {
        for (auto c = ' '; c < 127; ++c)
            detail::set_letter_glyph(static_cast<int16>(c));
    }

    static void blit_letter(int16 character, int32 clip_height,
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

        // If advance was requested, we will this with such information.
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

#if 0
        if (FAILED(SDL_BlitSurface(temp::alphabet_texture, &letter_rect,
                                   global::screen, &sdl_rect)))
        {
            PANIC("Bitting surface failed!");
        }
#elif 0
        auto temp_rect = SDL_Rect{ sdl_rect.x, sdl_rect.y, letter_rect.w, letter_rect.h };
        SDL_FillRect(global::screen, &temp_rect, global::foreground_hex_color);
#else
        // TODO: Not sure what to do if this does not work.
        ASSERT(global::screen->format->BytesPerPixel == 4);

        auto rmask = global::screen->format->Rmask;
        auto gmask = global::screen->format->Gmask;
        auto bmask = global::screen->format->Bmask;
        auto amask = global::screen->format->Amask;

        auto rshift = __builtin_ffsll(rmask / 0xFF) - 1;
        auto gshift = __builtin_ffsll(gmask / 0xFF) - 1;
        auto bshift = __builtin_ffsll(bmask / 0xFF) - 1;
        auto ashift = __builtin_ffsll(amask / 0xFF) - 1;

#if 1
        if(sdl_rect.y >= global::screen->h || sdl_rect.x >= global::screen->w)
            PANIC("Blitting completly outside the surface! This should never happen.");
#endif

        auto pixels = static_cast<uint32*>(global::screen->pixels);
        for(int32 y = sdl_rect.y; y < sdl_rect.y + letter_rect.h; ++y)
            for(int32 x = sdl_rect.x; x < sdl_rect.x + letter_rect.w; ++x)
            {
                if(x >= global::screen->w)
                    continue;

                if(y >= global::screen->h)
                    goto END;

                auto dest = pixels + y * global::screen->pitch / 4 + x;

                auto aph_tex_x = letter_rect.x + x - sdl_rect.x;
                auto aph_tex_y = letter_rect.y + y - sdl_rect.y;

                auto alpha = ((*(static_cast<uint32*>(temp::alphabet_texture->pixels) +
                                 aph_tex_y * temp::alphabet_texture->pitch / 4 +
                                 aph_tex_x)) & amask) >> ashift;
                auto alpha_real = static_cast<real32>(alpha) / 255.0f;

                auto dest_r = (global::background_hex_color & rmask) >> rshift;
                auto dest_g = (global::background_hex_color & gmask) >> gshift;
                auto dest_b = (global::background_hex_color & bmask) >> bshift;

                auto source_r = (global::foreground_hex_color & rmask) >> rshift;
                auto source_g = (global::foreground_hex_color & gmask) >> gshift;
                auto source_b = (global::foreground_hex_color & bmask) >> bshift;

                auto res_r = (1.0f - alpha_real) * dest_r + alpha_real * source_r;
                auto res_g = (1.0f - alpha_real) * dest_g + alpha_real * source_g;
                auto res_b = (1.0f - alpha_real) * dest_b + alpha_real * source_b;

                auto res = ((static_cast<uint32>(res_r + 0.5f) << rshift) |
                            (static_cast<uint32>(res_g + 0.5f) << gshift) |
                            (static_cast<uint32>(res_b + 0.5f) << bshift));

                *dest = res;
            }
#endif
    END:;
    }

    // TODO: Handle the copypaste, once it comes to blitting color surfaces.
    static void blit_letter_colored(int16 character, int32 clip_height,
                                    int32 X, int32 Y, int32* advance,
                                    graphics::rectangle const* viewport_rect)
    {
        blit_letter(character, clip_height, X, Y, advance, viewport_rect);
        return;
#if 0
        auto glyph = global::alphabet_colored_[character];
        auto sdl_rect = SDL_Rect {
            X + glyph.metrics.x_min,
            Y + glyph.metrics.y_min,
            10000,
            10000, // TODO: Who cares about W and H?
        };

        // If advance was requested, we will this with such information.
        if(advance)
            *advance = glyph.metrics.advance;

        if (FAILED(SDL_BlitSurface(glyph.texture,
                                   nullptr,
                                   global::screen,
                                   &sdl_rect)))
        {
            PANIC("Bitting surface failed!");
        }
#endif
    }

    // TODO(Cleanup): Make them more safe. The size should be some global setting.
    static int get_letter_width()
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

// TODO(EXPREIMENTAL): When comparing to SumblimeText, we move one pixel too
//                     much.  With this change it looks much better, but I have
//                     NO FREAKIN' IDEA WHY!!
    template<typename STR>
    static void print_text_line(editor::window const* window_ptr,
                                STR& text,
                                bool color,
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
        for (auto i = 0_u64; i < text_len; ++i)
        {
            auto text_idx = static_cast<int16>(text[i]);

            // TODO: This does not make much sense. Take a look at it!
            auto fixed_height = window_ptr->position.y + window_ptr->position.height - Y;

            // TODO: This looks like a reasonable default, doesn't it?
            auto advance = ::platform::get_letter_width();
            if(color)
                ::platform::blit_letter_colored(text_idx, fixed_height,
                                                X, Y, &advance,
                                                &window_ptr->position);
            else
                ::platform::blit_letter(text_idx, fixed_height,
                                        X, Y, &advance,
                                        &window_ptr->position);

            X += advance;


            if(static_cast<uint64>(cursor_idx) == i + 1)
            {
                cursor_x = X;
                cursor_y = Y;
            }
        }

        // TODO: Make sure that the cursor fits in the window.
        // Draw a cursor.
        if (cursor_idx >= 0)
        {
            auto rect = graphics::rectangle {
                static_cast<int32>(cursor_x),
                static_cast<int32>(window_ptr->position.y + y_offset +
                                   get_line_height() * line_nr - get_font_descent()),
                1,
                get_line_height()
            };

            draw_rectangle_on_screen(rect, graphics::make_color(global::foreground_hex_color));
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
        graphics::DrawSplittingLine({ 0, ::graphics::global::window_h - 17, ::graphics::global::window_w, 1 });

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


        // TODO: Make them global/const?
        auto horizontal_offset = 0;
        auto vertical_offset = 0;
        auto linum_horizontal_offset = 10;
        auto max_linum_digits_visible = std::to_string(
            std::min(b_point->first_line + number_of_displayed_lines + (b_point->starting_from_top ? 1 : 0),
                     b_point->buffer_ptr->size())).size();

        auto linum_rect = SDL_Rect{
            gap_w->position.x,
            gap_w->position.y,
            static_cast<int32>(max_linum_digits_visible) * get_letter_width() + 2 * linum_horizontal_offset,
            gap_w->position.height,
        };

        SDL_FillRect(global::screen, &linum_rect, global::background_hex_color);

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
            print_text_line(gap_w, s, false, lines_printed,
                            -1,
                            horizontal_offset + linum_horizontal_offset,
                            static_cast<int32>(first_line_offset) + vertical_offset,
                            b_point->starting_from_top);

            print_text_line(gap_w, *b_point->buffer_ptr->get_line(i), false, lines_printed,
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
