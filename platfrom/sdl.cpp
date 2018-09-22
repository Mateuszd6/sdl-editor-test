#include <SDL.h>
#include <SDL_ttf.h>

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

    static const int number_of_surfaces = 5;
    static TTF_Font* font;
    static SDL_Surface* alphabet[256];
    static SDL_Surface* alphabet_colored[256];
    static SDL_Surface* text_surface[number_of_surfaces];
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
    }

    static void destroy_font()
    {
        TTF_CloseFont(global::font);
        global::font = nullptr;
    }

    static void run_font_test()
    {
        if (!(global::text_surface[0] =
              TTF_RenderText_Solid(global::font,
                                   "This text is solid.",
                                   SDL_Color {0xF8, 0xF8, 0xF8, 0xFF}))
            || !(global::text_surface[1] =
                 TTF_RenderText_Shaded(global::font,
                                       "This text is shaded.",
                                       (SDL_Color){0xF8, 0xF8, 0xF8, 0xFF},
                                       // SDL_Color {0x15, 0x15, 0x80, 0xFF})) // 0x272822
                                       SDL_Color { 0x27, 0x28, 0x22, 0xFF}))
            || !(global::text_surface[2] =
                 TTF_RenderText_Blended(global::font,
                                        "This text is smoothed.",
                                        SDL_Color {0xF8, 0xF8, 0xF8, 0xFF}))
            || !(global::text_surface[3] =
                 TTF_RenderText_Blended(global::font,
                                        "This text is colored.",
                                        SDL_Color {0xF9, 0x26, 0x72, 0xFF}))
            || !(global::text_surface[4] =
                 TTF_RenderText_Blended(global::font,
                                        "static const auto font_size",
                                        SDL_Color {0xF9, 0x26, 0x72, 0xFF})))
        {
            // TODO(Debug): handle error here, perhaps print TTF_GetError at least.
            PANIC("SDL_TTF internal error. Game Over ;(");
        }

        for (auto c = 1; c < 128; ++c)
        {
            char letter[2];
            letter[0] = c;
            letter[1] = '\0';
            if (IS_NULL(::platform::global::alphabet[c] =
                        TTF_RenderText_Blended(global::font,
                                               letter,
                                               SDL_Color {0xF8, 0xF8, 0xF8, 0xFF})))
            {
                PANIC("SDL_TTF internal error. Game Over ;(");
            }
        }

        for (auto c = 1; c < 128; ++c)
        {
            char letter[2];
            letter[0] = c;
            letter[1] = '\0';
            if (IS_NULL(::platform::global::alphabet_colored[c] =
                        TTF_RenderText_Blended(global::font,
                                               letter,
                                               SDL_Color {0xFF, 0x00, 0x00, 0xFF})))
            {
                PANIC("SDL_TTF internal error. Game Over ;(");
            }
        }
    }

    static void blit_letter(int character, ::graphics::rectangle const& rect)
    {
        auto sdl_rect = SDL_Rect { rect.x, rect.y, rect.width, rect.height };
        if (FAILED(SDL_BlitSurface(global::alphabet[character],
                                   nullptr,
                                   global::screen,
                                   &sdl_rect)))
        {
            PANIC("Bitting surface failed!");
        }
    }

    static void blit_letter_colored(int character, ::graphics::rectangle const& rect)
    {
        auto sdl_rect = SDL_Rect { rect.x, rect.y, rect.width, rect.height };
        if (FAILED(SDL_BlitSurface(global::alphabet_colored[character],
                                   nullptr,
                                   global::screen,
                                   &sdl_rect)))
        {
            PANIC("Bitting surface failed!");
        }
    }

    // TODO(Cleanup): Make them more safe. The size should be some global setting.
    static int get_letter_height()
    {
        return global::alphabet[1]->h;
    }

    // TODO(Cleanup): Make them more safe. The size should be some global setting.
    static int get_letter_width()
    {
        return global::alphabet[1]->w;
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
