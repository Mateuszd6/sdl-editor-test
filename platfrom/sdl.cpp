#include <algorithm>

#include <SDL.h>
// #include <SDL_ttf.h>

// Freetype2:
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

// TODO(Cleanup): Try to use as less globals as possible...
namespace platform::global
{
    struct glyph_metrics
    {
        int32 x_min;
        int32 x_max;
        int32 y_min;
        int32 y_max;
        int32 advance;

        int32 width() { return x_max - x_min; }
        int32 height() { return y_max - y_min; }
    };

    struct glyph_data
    {
        SDL_Surface* texture;
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

#if 0
    static TTF_Font* font;
    static int32 font_ascent;
#endif

#if 0
    static SDL_Surface* alphabet[256];
    static SDL_Surface* alphabet_colored[256];
#else
    static glyph_data alphabet_[256];
    static glyph_data alphabet_colored_[256];
#endif

    static FT_Library library;
    static FT_Face face;

    static int32 line_height;

    static auto ft_font_size = 11 * 64;
}

namespace platform::detail
{
    static void set_letter_glyph(int16 letter)
    {
#if 0
        auto result = TTF_RenderGlyph_Blended(global::font, letter, SDL_Color { 0xF8, 0xF8, 0xF8, 0xFF });
        if(IS_NULL(result))
            PANIC("Could not render letter %d", letter);

        global::alphabet_[letter].texture = result;
        auto error = TTF_GlyphMetrics(global::font, letter,
                                      &global::alphabet_[letter].metrics.x_min,
                                      &global::alphabet_[letter].metrics.x_max,
                                      &global::alphabet_[letter].metrics.y_min,
                                      &global::alphabet_[letter].metrics.y_max,
                                      &global::alphabet_[letter].metrics.advance);

        if(error)
            PANIC("Could not load the glyph metrics!");


        // TODO: Fix this horrible thing!
        {
            global::alphabet_colored_[letter].texture =
                TTF_RenderGlyph_Blended(global::font, letter, SDL_Color { 0xF8, 0x10, 0x10, 0xFF });
            global::alphabet_colored_[letter].metrics = global::alphabet_[letter].metrics;
        }
#else
        auto error = FT_Load_Char(global::face, letter, FT_LOAD_RENDER);
        if(error)
            PANIC("Could not load char! %d", error);

        auto w = global::face->glyph->bitmap.width;
        auto h = global::face->glyph->bitmap.rows;
        auto bitmap = global::face->glyph->bitmap.buffer;

        // TODO: Make them global and static!!!
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        auto rmask = 0xff000000;
        auto gmask = 0x00ff0000;
        auto bmask = 0x0000ff00;
        auto amask = 0x000000ff;
#else
        auto rmask = 0x000000ff;
        auto gmask = 0x0000ff00;
        auto bmask = 0x00ff0000;
        auto amask = 0xff000000;
#endif

        // Make and fill the SDL Surface
        auto surface = SDL_CreateRGBSurface(0, w, h, 32,
                                            rmask, gmask, bmask, amask);
        if (surface == nullptr)
        {
            PANIC("SDL_CreateRGBSurface() failed: %s", SDL_GetError());
            exit(1);
        }
        uint8* pixels = (uint8*)surface->pixels;
        for(auto x = 0_u32; x < w; ++x)
            for(auto y = 0_u32; y < h; ++y)
                for(auto c = 0; c < 4; ++c)
                    pixels[4 * (y * w + x) + c] = bitmap[y * w + x];

        // Save the metrics provided by Freetype2 to my metric system.
        // TODO: x_max, y_max!!!
        auto metrics = global::face->glyph->metrics;
        global::alphabet_[letter].metrics.x_min = static_cast<int>(metrics.horiBearingX) / 64;
        global::alphabet_[letter].metrics.y_min = -static_cast<int>(metrics.horiBearingY) / 64;
        global::alphabet_[letter].metrics.advance = static_cast<int>(metrics.horiAdvance / 64);
        global::alphabet_[letter].texture = surface;

        // LOG_INFO("(%d;%d) x (%d;%d)    ->>> %d\n", x0, y0, x1, y1, adv);
#endif
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
        printf("dpi is: %f x %f\n", hdpi, vdpi);


        // TODO: Move this to another line.
        auto pixel_size = global::ft_font_size;
        auto error = FT_Set_Char_Size(global::face, global::ft_font_size, 0, hdpi, 0 /* vdpi */);
        if(error)
            PANIC("Setting the error failed.");
    }

    static void destroy_font()
    {
        // TODO: I dont want to free the font yet.
        return;
    }

    static void run_font_test()
    {
        for (auto c = 1; c < 128; ++c)
            detail::set_letter_glyph(c);

        for(auto i = 0; i < 128; ++i)
        {
            auto metr = global::alphabet_[i].metrics;
            if(global::alphabet_[i].texture)
            {
                printf("[%c]:\n\t(%d;%d) x (%d;%d)\n\t(%d;%d)\n",
                       i,
                       metr.x_min, metr.y_min, metr.x_max, metr.y_max,
                       global::alphabet_[i].texture->w, global::alphabet_[i].texture->h);
            }
        }


        global::line_height = global::face->size->metrics.height / 64;
#if 0
        BREAK();
#endif
    }

    static void blit_letter(int16 character, int32 clip_height,
                            int32 X, int32 Y, int32* advance)
    {
        auto glyph = global::alphabet_[character];
        auto sdl_rect = SDL_Rect {
            X + glyph.metrics.x_min,
            Y + glyph.metrics.y_min,
            10000,
            10000, // TODO: Who cares about W and H?
        };

        auto sdl_rect_viewport = SDL_Rect {
            0, 0, // -glyph.metrics.y_max, // global::font_ascent - glyph.metrics.y_max,
            1000, 1000, // glyph.texture->w, std::min(glyph.texture->h, clip_height),
        };

        if(advance)
        {
            *advance = glyph.metrics.advance;
            printf("ADVNCE: %d\n", *advance);
        }

        if (FAILED(SDL_BlitSurface(glyph.texture,
                                   &sdl_rect_viewport,
                                   global::screen,
                                   &sdl_rect)))
        {
            PANIC("Bitting surface failed!");
        }
    }

    static void blit_letter_colored(int16 character, int32 clip_height,
                            int32 X, int32 Y, int32* advance)
    {
        blit_letter(character, clip_height, X, Y, advance);
#if 0
        auto surface_to_blit = global::alphabet_colored_[character].texture;

        auto sdl_rect = SDL_Rect { rect.x, rect.y, rect.width, rect.height };
        auto sdl_rect_viewport = SDL_Rect {
            0, 0,
            surface_to_blit->w, std::min(surface_to_blit->h, clip_height),
        };

        if (FAILED(SDL_BlitSurface(surface_to_blit,
                                   &sdl_rect_viewport,
                                   global::screen,
                                   &sdl_rect)))
        {
            PANIC("Bitting surface failed!");
        }
#endif
    }

    // TODO(Cleanup): Make them more safe. The size should be some global setting.
    static int get_letter_height()
    {
        return global::alphabet_[1].texture->h;
    }

    // TODO(Cleanup): Make them more safe. The size should be some global setting.
    static int get_letter_width()
    {
        return global::alphabet_[1].texture->w;
    }

    static int32 get_line_height()
    {
        return global::line_height;
    }

    static void print_text_line_form_gap_buffer(editor::window const* window_ptr,
                                                gap_buffer const* line,
                                                int line_nr,
                                                int current_idx,
                                                int first_line_additional_offset,
                                                bool start_from_top)
    {
        auto text = line->to_c_str();
        graphics::print_text_line(window_ptr,
                                  reinterpret_cast<char const*>(text),
#ifdef GAP_BUF_SSO
                                  line->sso_enabled(),
#else
                                  false,
#endif
                                  line_nr,
                                  current_idx,
                                  first_line_additional_offset,
                                  start_from_top);

        std::free(reinterpret_cast<void*>(const_cast<int8*>(text)));
    }

    // TODO(Cleanup): Check if something here can fail, if not change the type to
    // void.
    static int redraw_window()
    {
        ::platform::global::screen = SDL_GetWindowSurface(::platform::global::window);
        if (!::platform::global::screen)
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
#if 1
        graphics::DrawSplittingLine({ 0, ::graphics::global::window_h - 17, ::graphics::global::window_w, 1 });
#endif

        mini_buffer_window->redraw(editor::global::current_window_idx == 0);
        main_window->redraw(editor::global::current_window_idx == 1);

        auto gap_w = editor::global::windows_arr + 1;
        auto b_point = &gap_w->buf_point;

        // TODO: There is still no space for the cutted line, and it can easilly be displayed here.
        auto number_of_displayed_lines = static_cast<uint64>(
            (gap_w->position.height - 2) / ::platform::get_line_height());

#if 0
        auto difference = (b_point->starting_from_top
                           ? static_cast<int64>(b_point->curr_line) - static_cast<int64>(b_point->first_line)
                           : static_cast<int64>(b_point->first_line) - static_cast<int64>(b_point->curr_line));
#endif

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

        auto lines_printed = 0_u64;
        auto first_line_offset = (b_point->starting_from_top
                                  ? 0
                                  : (gap_w->position.height - 2) -
                                  (::platform::get_line_height() * (number_of_displayed_lines + 1)));

        for(auto i = b_point->first_line + (b_point->starting_from_top ? 0 : -1);
            i < b_point->buffer_ptr->size();
            ++i)
        {
            print_text_line_form_gap_buffer(gap_w,
                                            b_point->buffer_ptr->get_line(i),
                                            lines_printed++,
                                            i == b_point->curr_line ? b_point->curr_idx : -1,
                                            first_line_offset,
                                            b_point->starting_from_top);

            // TODO: Check if this is correct in both cases.
            auto should_break = lines_printed >= number_of_displayed_lines + 1;
            if(should_break)
                break;
        }

        SDL_UpdateWindowSurface(::platform::global::window);
        return 0;
    }

}
