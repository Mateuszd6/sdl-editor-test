#include <algorithm>

#include <SDL.h>
#include <SDL_ttf.h>

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

    static const int number_of_surfaces = 5;
    static TTF_Font* font;
    static int32 font_ascent;

#if 0
    static SDL_Surface* alphabet[256];
    static SDL_Surface* alphabet_colored[256];
#else
    static glyph_data alphabet_[256];
    static glyph_data alphabet_colored_[256];
#endif
}

namespace platform::detail
{
    static void set_letter_glyph(int16 letter)
    {
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
        global::font = TTF_OpenFont(font_path, ::graphics::global::font_size);
        global::font_ascent = TTF_FontAscent(global::font);
    }

    static void destroy_font()
    {
        return; // TODO: I dont want to free the font yet.

        TTF_CloseFont(global::font);
        global::font = nullptr;
        global::font_ascent = 0;
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

        // BREAK();
    }

    static void blit_letter(int character,
                            int clip_height,
                            ::graphics::rectangle const& rect,
                            int* advance)
    {
        auto glyph = global::alphabet_[character];
        auto surface_to_blit = glyph.texture;
        auto sdl_rect = SDL_Rect { rect.x, rect.y, rect.width, rect.height };
        auto sdl_rect_viewport = SDL_Rect {
            glyph.metrics.x_min, global::font_ascent - glyph.metrics.y_max,
            1000, 1000, // surface_to_blit->w, std::min(surface_to_blit->h, clip_height),
        };

        *advance = glyph.metrics.advance;
        printf("ADVNCE: %d\n", *advance);

        if (FAILED(SDL_BlitSurface(surface_to_blit,
                                   &sdl_rect_viewport,
                                   global::screen,
                                   &sdl_rect)))
        {
            PANIC("Bitting surface failed!");
        }
    }

    static void blit_letter_colored(int character,
                                    int clip_height,
                                    ::graphics::rectangle const& rect,
                                    int* advance)
    {
        blit_letter(character, clip_height, rect, advance);
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
            (gap_w->position.height - 2) / ::platform::get_letter_height());

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
                                  (::platform::get_letter_height() * (number_of_displayed_lines + 1)));

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
