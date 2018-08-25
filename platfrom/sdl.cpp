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
    static SDL_Surface* alphabet['z' - 'a' + 1];
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

    static int get_letter_height()
    {
        return global::alphabet[1]->h;
    }

    static int get_letter_width()
    {
        return global::alphabet[1]->w;
    }

    static void print_text_line_form_gap_buffer(editor::window const* window_ptr,
                                                int line_nr,
                                                gap_buffer const* line,
                                                int current_idx)
    {
        auto text = line->to_c_str();
        graphics::print_text_line(window_ptr,
                                  line_nr,
                                  reinterpret_cast<char const*>(text),
                                  current_idx);

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

        LOG_DEBUG("Number of lines in the canvas: %ld. Starting from: %lu(%c), line: %lu)",
                  number_of_displayed_lines,
                  b_point->first_line,
                  b_point->starting_from_top ? 'v' : '^',
                  b_point->curr_line);

#if 1
        auto difference = (b_point->starting_from_top
                           ? static_cast<int64>(b_point->curr_line) - static_cast<int64>(b_point->first_line)
                           : static_cast<int64>(b_point->first_line) - static_cast<int64>(b_point->curr_line));
#else
        auto difference = (b_point->curr_line > b_point->first_line
                           ? b_point->curr_line - b_point->first_line
                           : b_point->first_line - b_point->curr_line);
#endif

        LOG_TRACE("____ CURR: %lu, FIRST LINE: %lu, DIFF: %ld ____",
                  b_point->curr_line,
                  b_point->first_line,
                  difference);

#if 0
        // TODO(NEXT): This is the place where orientation
        //             (buf_point.starting_from_top) should be changed.
        auto have_to_scroll = (b_point->starting_from_top
                               ? difference >= static_cast<int64>(number_of_displayed_lines)
                               : difference < 0);
        if(have_to_scroll)
        {
#if 0
            b_point.first_line = b_point.curr_line - (number_of_displayed_lines - 1);
#else
            if(b_point->starting_from_top)
            {
                b_point->starting_from_top = false;
                b_point->first_line = b_point->curr_line;
            }
            else
            {

            }
#endif
        }
#else
        if(b_point->curr_line < b_point->first_line)
        {
            b_point->starting_from_top = true;
            b_point->first_line = b_point->curr_line;
        }
        else if(b_point->curr_line >= b_point->first_line + number_of_displayed_lines - 1)
        {
            b_point->starting_from_top = false;
            b_point->first_line = b_point->curr_line - number_of_displayed_lines + 1;
        }
#endif

#if 0
        auto lines_printed = b_point->starting_from_top ? 0_u64 : number_of_displayed_lines;
        for(auto i = b_point->first_line;
            0 <= i && i < b_point->buffer_ptr->size();
            b_point->starting_from_top ? ++i : --i)
        {
            LOG_TRACE("Printing line [%c]: %lu", b_point->starting_from_top ? 'v' : '^', lines_printed);
            print_text_line_form_gap_buffer(gap_w,
                                            b_point->starting_from_top ? lines_printed++ : lines_printed--,
                                            b_point->buffer_ptr->get_line(i),
                                            i == b_point->curr_line ? b_point->curr_idx : -1);

            auto should_break = (b_point->starting_from_top
                                 ? (lines_printed >= number_of_displayed_lines)
                                 : (lines_printed < 0));
            if(should_break)
                break;
        }
#else
        auto lines_printed = 0_u64;
        for(auto i = b_point->first_line;
            i < b_point->buffer_ptr->size();
            ++i)
        {
            LOG_TRACE("Printing line [%c]: %lu", b_point->starting_from_top ? 'v' : '^', lines_printed);
            print_text_line_form_gap_buffer(gap_w,
                                            lines_printed++,
                                            b_point->buffer_ptr->get_line(i),
                                            i == b_point->curr_line ? b_point->curr_idx : -1);

            auto should_break = (b_point->starting_from_top
                                 ? (lines_printed >= number_of_displayed_lines)
                                 : (lines_printed < 0));
            if(should_break)
                break;
        }
#endif

#if 0
        auto gap_w = editor::global::windows_arr + 1;
        for (auto i = 0_u64; i < 128_u64; ++i)
        {
            print_text_line_form_gap_buffer(
                gap_w,
                i,
                gap_w->buffer_ptr->lines + i,
                (gap_w->buffer_ptr->curr_line == i ? gap_w->buffer_ptr->curr_index : -1));
        }

        if (!(editor::global::windows_arr + 1)->contains_buffer)
        {
            if ((editor::global::windows_arr + 2)->contains_buffer
                && (editor::global::windows_arr + 4)->contains_buffer
                && (editor::global::windows_arr + 5)->contains_buffer)
            {
                auto left_w = editor::global::windows_arr + 2;
                auto right_w = editor::global::windows_arr + 4;
                auto gap_w = editor::global::windows_arr + 1;

                ::graphics::print_text_line(left_w, 0, "#include <iostream>", -1);
                ::graphics::print_text_line(left_w, 2, "int main()", -1);
                ::graphics::print_text_line(left_w, 3, "{", -1);
                ::graphics::print_text_line(left_w, 4, "    const auto foobar = \"Hello world!\";", -1);
                ::graphics::print_text_line(left_w, 5, "    std::cout << foobar << endl;", -1);
                ::graphics::print_text_line(left_w, 7, "    return 0;", -1);
                ::graphics::print_text_line(left_w, 8, "}", -1);

                ::graphics::print_text_line(right_w, 0, "<!DOCTYPE html>", -1);
                ::graphics::print_text_line(right_w, 1, "<html>", -1);
                ::graphics::print_text_line(right_w, 2, "  <body>", -1);
                ::graphics::print_text_line(right_w, 3, "    <h1>Example heading</h1>", -1);
                ::graphics::print_text_line(right_w, 4, "    <p>Example paragraph</p>", -1);
                ::graphics::print_text_line(right_w, 5, "  </body>", -1);
                ::graphics::print_text_line(right_w, 6, "</html>", -1);
            }
        }
#endif

        SDL_UpdateWindowSurface(::platform::global::window);
        return 0;
    }

}
