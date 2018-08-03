#include "config.h"

// TODO(Cleanup): remove from here as many as possible.
#include "text/gap_buffer.hpp"
#include "editor/buffer.hpp"
#include "editor/window.hpp"
#include "graphics/graphics.hpp"
#include "platfrom/platform.hpp"

// TODO(Cleaup): Only for unity build.
#include "text/gap_buffer.cpp"
#include "editor/buffer.cpp"
#include "editor/window.cpp"
#include "graphics/graphics.cpp"
#include "platfrom/platform.cpp"

namespace editor_window_utility
{
    namespace detail
    {
        static void ResizeIthWindowLeft(editor::window* parent_window,
                                        int index_in_parent)
        {
            // Cannot be 0, because we move left.
            if(index_in_parent == 0)
            {
                LOG_WARN("Cannot move left.");
                return;
            }

            auto prev_split = (index_in_parent > 1
                               ? parent_window->splits_percentages[index_in_parent - 2]
                               : 0.0f);

            // This does not work, because nested windows spoit them a lot.
#if 0
            if (parent_window->split_windows[index_in_parent - 1]
                ->GetScreenPercForSplitType(window_split::WIN_SPLIT_HORIZONTAL) >= 0.2f)
#endif

                if (parent_window->splits_percentages[index_in_parent - 1] - prev_split
                    >= MIN_PERCANTAGE_WINDOW_SPLIT)
                {
                    parent_window->splits_percentages[index_in_parent - 1] -= 0.01f;
                    parent_window->UpdateSize(parent_window->position);
                    parent_window->Redraw(false); // Split window, cannot select.
                }
                else
                    LOG_WARN("Left window is too small!!");
        }

        static void ResizeIthWindowRight(editor::window* parent_window,
                                         int index_in_parent)
        {

            // Cannot be max, because we move right.
            if (index_in_parent == parent_window->number_of_windows -1)
            {
                LOG_WARN("Cannot move right.");
                return;
            }

            auto next_split = (index_in_parent < parent_window->number_of_windows -1
                               ? parent_window->splits_percentages[index_in_parent + 1]
                               : 1.0f);

            if (next_split - parent_window->splits_percentages[index_in_parent]
                >= MIN_PERCANTAGE_WINDOW_SPLIT)
            {
                parent_window->splits_percentages[index_in_parent] += 0.01f;
                parent_window->UpdateSize(parent_window->position);
                parent_window->Redraw(false);
            }
            else
                LOG_WARN("Right window is too small!!");
        }
    }

    // NOTE: [curr_window] is heavily assumed to be the selected window.
    // TODO: Find better name.
    // TODO: Remove unused atrribute!
    __attribute__ ((unused))
    static void ResizeWindowAux(editor::window* curr_window,
                                editor::window_split split_type,
                                editor::window_traverse_mode direction)
    {
        // TODO(Cleaup): Explain.
        auto right_parent = curr_window->parent_ptr;
        auto child = curr_window;
        while(right_parent
              && (right_parent->split != split_type
                  || (direction == editor::window_traverse_mode::WIN_TRAVERSE_BACKWARDS
                      &&  child->IsFirstInParent())
                  || (direction == editor::window_traverse_mode::WIN_TRAVERSE_FORWARD
                      && child->IsLastInParent())))
        {
            ASSERT(!right_parent->contains_buffer);

            child = right_parent;
            right_parent = right_parent->parent_ptr;
        }

        if(!right_parent)
        {
            // TODO(Cleaup): Explain.
            LOG_WARN("Could not resize, because right border was not found");
            return;
        }
        ASSERT(!right_parent->contains_buffer);

        auto index_in_parent = -1;
        for (auto i = 0; i < right_parent->number_of_windows; ++i)
            if (right_parent->split_windows[i] == child)
            {
                index_in_parent = i;
                break;
            }
        ASSERT(index_in_parent >= 0);

        switch(direction)
        {
            case (editor::window_traverse_mode::WIN_TRAVERSE_BACKWARDS):
                detail::ResizeIthWindowLeft(right_parent, index_in_parent);
                break;
            case (editor::window_traverse_mode::WIN_TRAVERSE_FORWARD):
                detail::ResizeIthWindowRight(right_parent, index_in_parent);
                break;

            default:
                UNREACHABLE();
        }
    }
}

// TODO(Cleanup): Would be nice to add them using regular, add-window/buffer API.
static void InitializeFirstWindow()
{
    editor::global::number_of_buffers = 2;
    editor::global::buffers[0] = {}; // TODO(Next): Initailize buffers.

    // minibufer.
    editor::global::buffers[1] = {}; // TODO(Next): Initailize buffers.

    editor::global::number_of_windows = 2;

    // Window with index 0 contains minibuffer. And is drawn separetely.
    editor::global::windows_arr[0] = editor::window {
        .contains_buffer = 1,
        .buffer_ptr = editor::global::buffers,
        .parent_ptr = nullptr,
        .position = graphics::rectangle {
            0, ::graphics::global::window_h - 17 + 1,
            ::graphics::global::window_w, 17
        }
    };

    // Window with index 1 is main window.
    editor::global::windows_arr[1] = editor::window {
        .contains_buffer = 1,
        .buffer_ptr = editor::global::buffers + 1,
        .parent_ptr = nullptr,
        .position = graphics::rectangle {
            0, 0,
            ::graphics::global::window_w, ::graphics::global::window_h - 17
        }
    };

    editor::global::current_window_idx = 1;
}

// TODO(Cleanup): Create namespace application and move resize and redraw window
//                functions and all globals there!
// TODO(Cleanup): Check if something here can fail, if not change the type to
//                void.
static int ResizeWindow()
{
    editor::global::windows_arr[0].UpdateSize(
        graphics::rectangle {
                0, ::graphics::global::window_h - 17 + 1,
                ::graphics::global::window_w, 17
        });

    editor::global::windows_arr[1].UpdateSize(
        graphics::rectangle {
            0, 0,
            ::graphics::global::window_w, ::graphics::global::window_h - 17
        });

    return 0;
}

static void InitTextGlobalSutff()
{
    auto font_test = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSansMono.ttf",
                                  ::graphics::global::font_size);

    if (!(::platform::global::text_surface[0] =
          TTF_RenderText_Solid(font_test,
                               "This text is solid.",
                               SDL_Color {0xF8, 0xF8, 0xF8, 0xFF}))
        || !(::platform::global::text_surface[1] =
             TTF_RenderText_Shaded(font_test,
                                   "This text is shaded.",
                                   (SDL_Color){0xF8, 0xF8, 0xF8, 0xFF},
                                   // SDL_Color {0x15, 0x15, 0x80, 0xFF})) // 0x272822
                                   SDL_Color { 0x27, 0x28, 0x22, 0xFF}))
        || !(::platform::global::text_surface[2] =
             TTF_RenderText_Blended(font_test,
                                    "This text is smoothed.",
                                    SDL_Color {0xF8, 0xF8, 0xF8, 0xFF}))
        || !(::platform::global::text_surface[3] =
             TTF_RenderText_Blended(font_test,
                                    "This text is colored.",
                                    SDL_Color {0xF9, 0x26, 0x72, 0xFF}))
        || !(::platform::global::text_surface[4] =
             TTF_RenderText_Blended(font_test,
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
                    TTF_RenderText_Blended(font_test,
                                           letter,
                                           SDL_Color {0xF8, 0xF8, 0xF8, 0xFF})))
        {
            PANIC("SDL_TTF internal error. Game Over ;(");
        }
    }

    TTF_CloseFont(font_test);
    font_test = nullptr;
}

static void PrintTextLine(editor::window const* window_ptr,
                          int line_nr, // First visible line of the buffer is 0.
                          char const* text,
                          int cursor_idx) // gap_buffer const* line
{
    for (auto i = 0; text[i]; ++i)
    {
        auto text_idx = static_cast<int>(text[i]);
        auto draw_rect = SDL_Rect {
            // Only for monospace fonts.
            window_ptr->position.x + 2 + ::platform::global::alphabet[text_idx]->w * i,
            window_ptr->position.y + 2 + ::platform::global::text_surface[0]->h * line_nr,
            ::platform::global::alphabet[text_idx]->w,
            ::platform::global::alphabet[text_idx]->h
        };

        if (draw_rect.x + draw_rect.w >
            window_ptr->position.x + window_ptr->position.width
            || draw_rect.y + draw_rect.h >
            window_ptr->position.y + window_ptr->position.height)
        {
            return;
        }

        if (FAILED(SDL_BlitSurface(::platform::global::alphabet[text_idx], nullptr,
                                   ::platform::global::screen, &draw_rect)))
        {
            PANIC("Bitting surface failed!");
        }
    }

    if (cursor_idx >= 0)
    {
        auto rect = SDL_Rect {
            window_ptr->position.x + 2 +
            ::platform::global::alphabet[static_cast<int>('a')]->w * cursor_idx,
            window_ptr->position.y + 2 + ::platform::global::text_surface[0]->h * line_nr,
            2,
            ::platform::global::alphabet[static_cast<int>('a')]->h
        };

        SDL_FillRect(::platform::global::screen, &rect, 0xF8F8F8FF);
    }
}

static void PrintTextLineFromGapBuffer(editor::window const* window_ptr,
                                       int line_nr, // First visible line of the buffer is 0.
                                       gap_buffer const* line,
                                       int current_idx)
{
    auto text = line->to_c_str();
    PrintTextLine(window_ptr,
                  line_nr,
                  reinterpret_cast<char const*>(text),
                  current_idx);

#if 0 // Debug stuff.
    system("clear");
    line->DEBUG_print_state();
#endif

    std::free(reinterpret_cast<void *>(const_cast<unsigned char*>(text)));
}

// TODO(Cleanup): Check if something here can fail, if not change the type to
// void.
static int RedrawWindow()
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

    mini_buffer_window->Redraw(editor::global::current_window_idx == 0);
    main_window->Redraw(editor::global::current_window_idx == 1);

    if (!(editor::global::windows_arr + 1)->contains_buffer)
    {
        if ((editor::global::windows_arr + 2)->contains_buffer
            && (editor::global::windows_arr + 4)->contains_buffer
            && (editor::global::windows_arr + 5)->contains_buffer)
        {
            auto left_w = editor::global::windows_arr + 2;
            auto right_w = editor::global::windows_arr + 4;
            auto gap_w = editor::global::windows_arr + 5;

            PrintTextLine(left_w, 0, "#include <iostream>", -1);
            PrintTextLine(left_w, 2, "int main()", -1);
            PrintTextLine(left_w, 3, "{", -1);
            PrintTextLine(left_w, 4, "    const auto foobar = \"Hello world!\";", -1);
            PrintTextLine(left_w, 5, "    std::cout << foobar << endl;", -1);
            PrintTextLine(left_w, 7, "    return 0;", -1);
            PrintTextLine(left_w, 8, "}", -1);

            PrintTextLine(right_w, 0, "<!DOCTYPE html>", -1);
            PrintTextLine(right_w, 1, "<html>", -1);
            PrintTextLine(right_w, 2, "  <body>", -1);
            PrintTextLine(right_w, 3, "    <h1>Example heading</h1>", -1);
            PrintTextLine(right_w, 4, "    <p>Example paragraph</p>", -1);
            PrintTextLine(right_w, 5, "  </body>", -1);
            PrintTextLine(right_w, 6, "</html>", -1);

            for (auto i = 0_u64; i < 128_u64; ++i)
            {
                PrintTextLineFromGapBuffer(gap_w,
                                           i,
                                           gap_w->buffer_ptr->lines + i,
                                           (gap_w->buffer_ptr->curr_line == i
                                            ? gap_w->buffer_ptr->curr_index
                                            : -1));
            }
        }
    }

    SDL_UpdateWindowSurface(::platform::global::window);
    return 0;
}

// If traverse is forward, we pick the first window in the subtree of the
// root_window, otherwise we pick the last one (we always go either left or
// right recusively).
static editor::window *GetFirstOrLastWindowInSubtree(editor::window* root_window,
                                                     editor::window_traverse_mode traverse)
{
    // If root window is a bufer, it is just selected.
    if (root_window->contains_buffer)
        return root_window;

    auto next_window = root_window;
    auto window_to_pick_idx = 0;

    if (traverse == editor::window_traverse_mode::WIN_TRAVERSE_FORWARD)
        window_to_pick_idx = 0;
    else
    {
        ASSERT (traverse == editor::window_traverse_mode::WIN_TRAVERSE_BACKWARDS);
        window_to_pick_idx = next_window->number_of_windows - 1;
    }

    while (!next_window->contains_buffer)
        next_window = next_window->split_windows[window_to_pick_idx];

    return next_window;
}

static editor::window *GetNextOrPrevActiveWindow(editor::window const*current_window,
                                                 editor::window_traverse_mode traverse)
{
    auto parent = current_window->parent_ptr;
    if (!parent)
        return GetFirstOrLastWindowInSubtree(editor::global::windows_arr + 1, traverse);

    // Parent must be splited window.
    ASSERT(!parent->contains_buffer);
    auto idx_in_parent = current_window->GetIndexInParent();

    // TODO(Cleanup): Compress this better!
    if (traverse == editor::window_traverse_mode::WIN_TRAVERSE_FORWARD)
    {
        if (idx_in_parent + 1 >= parent->number_of_windows)
        {
            // Last window in this split - we must go to the upper window and search
            // the next one recusively.
            return GetFirstOrLastWindowInSubtree(
                GetNextOrPrevActiveWindow(parent, traverse),
                traverse);
        }
        else
        {
            return GetFirstOrLastWindowInSubtree(
                parent->split_windows[idx_in_parent + 1],
                traverse);
        }
    }
    else
    {
        ASSERT(traverse == editor::window_traverse_mode::WIN_TRAVERSE_BACKWARDS);
        if (idx_in_parent == 0)
        {
            // First window in this split - we must go to the parent window and
            // search the previous one recusively.
            return GetFirstOrLastWindowInSubtree(
                GetNextOrPrevActiveWindow(parent, traverse),
                traverse);
        }
        else
        {
            return GetFirstOrLastWindowInSubtree(
                parent->split_windows[idx_in_parent - 1],
                traverse);
        }
    }
}

// If go_forward is true we switch to the next widnow, otherwise we switch to
// the previous one.
static void SwitchWindow(editor::window_traverse_mode traverse)
{
    if (editor::global::current_window_idx == 0)
    {
        LOG_WARN("Cannot switch window in minibuffer!");
        return;
    }

    // TODO(Cleanup): Do we assume there is at least one buffer at a time?
    // TODO(Cleanup): What if there is just minibuffer?
    ASSERT(editor::global::number_of_buffers);

    auto next_window =
        GetNextOrPrevActiveWindow(editor::global::windows_arr + editor::global::current_window_idx,
                                  traverse);
    ASSERT(next_window);
    editor::global::current_window_idx = next_window - editor::global::windows_arr;
}

// TODO: Remove unused atrribute!
__attribute__ ((unused))
static void SwitchToMiniBuffer()
{
    if (editor::global::current_window_idx == 0)
    {
        LOG_WARN("Already in minibuffer");
        return;
    }

    editor::global::window_idx_before_entering_minibuffer = editor::global::current_window_idx;
    editor::global::current_window_idx = 0;
}

// TODO: Remove unused atrribute!
__attribute__ ((unused))
static void SwitchOutFromMiniBuffer()
{
    if (editor::global::current_window_idx != 0)
    {
        LOG_WARN("Not in minibuffer");
        return;
    }

    editor::global::current_window_idx
        = editor::global::window_idx_before_entering_minibuffer;
}

static int HandleEvent(const SDL_Event &event)
{
    auto shouldQuit = 0;

    switch (event.type)
    {
        case SDL_QUIT:
        {
            LOG_INFO("MSG ---> SDL_QUIT");
            shouldQuit = 1;
        } break;

        case SDL_KEYDOWN:
        {
#if 0
            // W - Switch window.
            if (event.key.keysym.sym == 119)
            {
                SwitchWindow(window_traverse_mode::WIN_TRAVERSE_FORWARD);
            }
            // Q - Switch window backwards.
            else if (event.key.keysym.sym == 113)
            {
                SwitchWindow(window_traverse_mode::WIN_TRAVERSE_BACKWARDS);
            }

            // [<-] - Move left current window border.
            else if (event.key.keysym.sym == 1073741904)
            {
                auto curr_window = (global::windows_arr + global::current_window_idx);
                editor_window_utility
                    ::ResizeWindowAux(curr_window,
                                      window_split::WIN_SPLIT_HORIZONTAL,
                                      window_traverse_mode::WIN_TRAVERSE_BACKWARDS);
            }

            // [->] - Move right current window border.
            else if (event.key.keysym.sym == 1073741903)
            {
                auto curr_window = (global::windows_arr + global::current_window_idx);
                editor_window_utility
                    ::ResizeWindowAux(curr_window,
                                      window_split::WIN_SPLIT_HORIZONTAL,
                                      window_traverse_mode::WIN_TRAVERSE_FORWARD);
            }

            // [v] - Move down current window border.
            else if (event.key.keysym.sym == 1073741905)
            {
                auto curr_window = (global::windows_arr + global::current_window_idx);
                editor_window_utility
                    ::ResizeWindowAux(curr_window,
                                      window_split::WIN_SPLIT_VERTCAL,
                                      window_traverse_mode::WIN_TRAVERSE_FORWARD);
            }

            // [^] - Move up current window border.
            else if (event.key.keysym.sym == 1073741906)
            {
                auto curr_window = (global::windows_arr + global::current_window_idx);
                editor_window_utility
                    ::ResizeWindowAux(curr_window,
                                      window_split::WIN_SPLIT_VERTCAL,
                                      window_traverse_mode::WIN_TRAVERSE_BACKWARDS);
            }


            // X - Focus the minibuffer.
            else if (event.key.keysym.sym == 120)
            {
                SwitchToMiniBuffer();
            }
            // G - Quit minibuffer.
            else if (event.key.keysym.sym == 103)
            {
                SwitchOutFromMiniBuffer();
            }

            // H - Split horizontally.
            else if (event.key.keysym.sym == 104)
            {
                if (global::current_window_idx != 0)
                {
                    global::windows_arr[global::current_window_idx].
                        SplitWindow(window_split::WIN_SPLIT_HORIZONTAL);
                }
                else
                {
                    LOG_WARN("Cannot split minibuffer");
                }
            }
            // V - Split vertically.
            else if (event.key.keysym.sym == 118)
            {
                if (global::current_window_idx != 0)
                {
                    global::windows_arr[global::current_window_idx].
                        SplitWindow(window_split::WIN_SPLIT_VERTCAL);
                }
                else
                    LOG_WARN("Cannot split minibuffer");
            }

            // D - Delete window.
            else if (event.key.keysym.sym == 100)
            {
                auto curr_win = global::windows_arr + global::current_window_idx;
                if (curr_win->parent_ptr)
                {
                    auto idx_in_parent = curr_win->GetIndexInParent();

                    if (idx_in_parent == 0)
                        PANIC("Not supported yet!");
                    else
                    {
                        curr_win->parent_ptr->splits_percentages[idx_in_parent - 1] =
                            curr_win->parent_ptr->splits_percentages[idx_in_parent];

                        for (auto i = idx_in_parent;
                             i < curr_win->parent_ptr->number_of_windows;
                             ++i)
                        {
                            curr_win->parent_ptr->splits_percentages[i] =
                                curr_win->parent_ptr->splits_percentages[i + 1];

                            curr_win->parent_ptr->split_windows[i] =
                                curr_win->parent_ptr->split_windows[i + 1];
                        }

                        curr_win->parent_ptr->number_of_windows--;
                    }
                }
                else
                    LOG_WARN("Cannot delete this window.");
            }

            if (global::windows_arr[global::current_window_idx].contains_buffer)
            {
                LOG_INFO("Current window: %d (with color: %x)",
                         global::current_window_idx,
                         global::windows_arr[global::current_window_idx]
                         .buffer_ptr->color);
            }
#elif 0
            LOG_INFO("MSG ---> KEY DOWN %d", event.key.keysym.sym);
#else
            auto character = '\0';
            auto arrow = 0;
            auto backspace = false;
            auto del = false;

            switch (event.key.keysym.sym)
            {

                case SDLK_a:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'A' : 'a';
                    break;
                case SDLK_b:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'B' : 'b';
                    break;
                case SDLK_c:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'C' : 'c';
                    break;
                case SDLK_d:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'D' : 'd';
                    break;
                case SDLK_e:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'E' : 'e';
                    break;
                case SDLK_f:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'F' : 'f';
                    break;
                case SDLK_g:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'G' : 'g';
                    break;
                case SDLK_h:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'H' : 'h';
                    break;
                case SDLK_i:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'I' : 'i';
                    break;
                case SDLK_j:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'J' : 'j';
                    break;
                case SDLK_k:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'K' : 'k';
                    break;
                case SDLK_l:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'L' : 'l';
                    break;
                case SDLK_m:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'M' : 'm';
                    break;
                case SDLK_n:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'N' : 'n';
                    break;
                case SDLK_o:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'O' : 'o';
                    break;
                case SDLK_p:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'P' : 'p';
                    break;
                case SDLK_q:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'Q' : 'q';
                    break;
                case SDLK_r:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'R' : 'r';
                    break;
                case SDLK_s:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'S' : 's';
                    break;
                case SDLK_t:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'T' : 't';
                    break;
                case SDLK_u:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'U' : 'u';
                    break;
                case SDLK_v:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'V' : 'v';
                    break;
                case SDLK_w:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'W' : 'w';
                    break;
                case SDLK_x:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'X' : 'x';
                    break;
                case SDLK_y:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'Y' : 'y';
                    break;
                case SDLK_z:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? 'Z' : 'z';
                    break;
                case SDLK_0:
                    character = '0';
                    break;
                case SDLK_1:
                    character = '1';
                    break;
                case SDLK_2:
                    character = '2';
                    break;
                case SDLK_3:
                    character = '3';
                    break;
                case SDLK_4:
                    character = '4';
                    break;
                case SDLK_5:
                    character = '5';
                    break;
                case SDLK_6:
                    character = '6';
                    break;
                case SDLK_7:
                    character = '7';
                    break;
                case SDLK_8:
                    character = '8';
                    break;
                case SDLK_9:
                    character = '9';
                    break;
                case SDLK_SPACE:
                    character = ' ';
                    break;

                case SDLK_BACKSPACE:
                    backspace = true;
                    break;
                case SDLK_DELETE:
                    del = true;
                    break;

                case SDLK_RIGHT:
                    arrow = 1;
                    break;
                case SDLK_LEFT:
                    arrow = 2;
                    break;
                case SDLK_UP:
                    arrow = 3;
                    break;
                case SDLK_DOWN:
                    arrow = 4;
                    break;

                default:
                    break;
            }

            auto succeeded = false;
            if (character != '\0')
            {
                (editor::global::windows_arr + 5)->buffer_ptr->insert_character(character);
            }
            else if (arrow == 1)
            {
                (editor::global::windows_arr + 5)->buffer_ptr->move_forward_curr_line();
            }
            else if (arrow == 2)
            {
                (editor::global::windows_arr + 5)->buffer_ptr->move_backward_curr_line();
            }
            else if (arrow == 3)
            {
                (editor::global::windows_arr + 5)->buffer_ptr->move_line_up();
            }
            else if (arrow == 4)
            {
                (editor::global::windows_arr + 5)->buffer_ptr->move_line_down();
            }
            else if (backspace)
            {
                (editor::global::windows_arr + 5)->buffer_ptr->delete_char_at_cursor_backward();
            }
            else if (del)
            {
                (editor::global::windows_arr + 5)->buffer_ptr->delete_char_at_cursor_forward();
            }

            if (!succeeded)
            {
                LOG_WARN("Could not make the operation!");
            }
#endif
        } break;

        case SDL_KEYUP:
        {
            LOG_INFO("MSG ---> KEY UP %d", event.key.keysym.sym);
        } break;

        case SDL_WINDOWEVENT:
        {
            switch (event.window.event)
            {
                case SDL_WINDOWEVENT_SHOWN:
                {
                    LOG_INFO("MSG ---> Window %d shown",
                             event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_HIDDEN:
                {
                    LOG_INFO("MSG ---> Window %d hidden",
                             event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_EXPOSED:
                {
                    LOG_INFO("MSG ---> Window %d exposed",
                             event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_MOVED:
                {
                    LOG_INFO("MSG ---> Window %d moved to %d,%d",
                             event.window.windowID,
                             event.window.data1,
                             event.window.data2);
                } break;

                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                {
                    LOG_INFO("MSG ---> Window %d resized",
                             event.window.windowID);
                    ResizeWindow();
                } break;

                case SDL_WINDOWEVENT_MINIMIZED:
                {
                    LOG_INFO("MSG ---> Window %d minimized",
                             event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_MAXIMIZED:
                {
                    LOG_INFO("MSG ---> Window %d maximized",
                             event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_RESTORED:
                {
                    LOG_INFO("MSG ---> Window %d restored",
                             event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_ENTER:
                {
                    LOG_INFO("MSG ---> Mouse entered window %d",
                             event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_LEAVE:
                {
                    LOG_INFO("MSG ---> Mouse left window %d",
                             event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_FOCUS_GAINED:
                {
                    LOG_INFO("MSG ---> Window %d gained keyboard focus",
                             event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_FOCUS_LOST:
                {
                    LOG_INFO("MSG ---> Window %d lost keyboard focus",
                             event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_CLOSE:
                {
                    LOG_INFO("MSG ---> Window %d closed",
                             event.window.windowID);
                } break;

#if SDL_VERSION_ATLEAST(2, 0, 5)
                case SDL_WINDOWEVENT_TAKE_FOCUS:
                {
                    LOG_INFO("MSG ---> Window %d is offered a focus",
                             event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_HIT_TEST:
                {
                    LOG_INFO("MSG ---> Window %d has a special hit test",
                             event.window.windowID);
                } break;
#endif

                default:
                {
                    LOG_INFO("MSG ---> Window %d got unknown event %d",
                             event.window.windowID,
                             event.window.event);
                } break;
            }
        } break;

        default:
            break;
    }

    return shouldQuit;
}

static int InitSDL()
{
    // TODO(Platform): Possibly inicialize with SDL_INIT_EVENTS?
    if (FAILED(SDL_Init(SDL_INIT_VIDEO)))
    {
        // TODO(Debug): Logging.
        return -1;
    }

    if (FAILED(TTF_Init()))
    {
        // TODO(Debug): Logging.
        return -1;
    }

    return 0;
}

static int InitWindow(const int width, const int height)
{
    // NOTE(Testing): Sometimes it is good to set width and height to 0, 0 and
    // test what happens when window is smallest possible.
    ::platform::global::window = SDL_CreateWindow("Editor",
                                                  SDL_WINDOWPOS_UNDEFINED,
                                                  SDL_WINDOWPOS_UNDEFINED,
                                                  width, height, 0);
                                                  // 0, 0, 0); // To test corner-cases.

    if (!::platform::global::window)
    {
        // TODO(Debug): Logging.
        return -1;
    }

    // Instead of creating a renderer, draw directly to the screen.
    ::platform::global::screen = SDL_GetWindowSurface(::platform::global::window);

    if (!::platform::global::screen)
    {
        // TODO(Debug): Logging.
        return -1;
    }

    return 0;
}

// Makes sure that the main window tree is done correctly. Inside horizontal
// split window cannot be another horizontal etc...
static bool Validate(editor::window *window)
{
    if (window->contains_buffer)
        return true;
    else
    {
        auto split = window->split;
        for (auto i = 0; i < window->number_of_windows; ++i)
            if (window->split_windows[i]->split == split)
                return false;

        auto invalid = false;
        for (auto i = 0; i < window->number_of_windows; ++i)
            if (!Validate(window->split_windows[i]))
                invalid = true;

        return !invalid;
    }
}

int main(void)
{
    if (FAILED(InitSDL()))
    {
        // TODO(Debug): Logging.
        exit(1);
    }

    // NOTE: Some system diagnostics.
    // Declare display mode structure to be filled in.
    SDL_DisplayMode current;
    if (SDL_GetCurrentDisplayMode(0, &current) != 0)
        LOG_INFO("Could not get display mode for video display #%d: %s",
                 0, SDL_GetError());
    else
        // On success, print the current display mode.
        LOG_INFO("Display #%d: current display mode is %dx%dpx @ %dhz.",
                 0, current.w, current.h, current.refresh_rate);

    // The size of the window to render.
    ::graphics::global::window_w = current.w / 3;
    ::graphics::global::window_h = current.h / 2;

    auto window_x = current.w / 2 - ::graphics::global::window_w / 2;
    auto window_y = current.h / 2 - ::graphics::global::window_h / 2;

    // TODO(Platform): Use values based on screen width/height, or let user set
    // his own.
    if (FAILED(InitWindow(::graphics::global::window_w, ::graphics::global::window_h)))
    {
        // TODO(Debug): Logging.
        exit(1);
    }
    SDL_SetWindowPosition(::platform::global::window, window_x, window_y);

    InitializeFirstWindow();

#if 1
    editor::global::windows_arr[editor::global::current_window_idx]
        .SplitWindow(editor::window_split::WIN_SPLIT_HORIZONTAL);
    SwitchWindow(editor::window_traverse_mode::WIN_TRAVERSE_FORWARD);
    editor::global::windows_arr[editor::global::current_window_idx]
        .SplitWindow(editor::window_split::WIN_SPLIT_VERTCAL);
    SwitchWindow(editor::window_traverse_mode::WIN_TRAVERSE_FORWARD);
#endif

    InitTextGlobalSutff();

    for (;;)
    {
        auto width = 0;
        auto height = 0;
        SDL_GetWindowSize(::platform::global::window, &width, &height);
        LOG_INFO("Current window size: %d x %d", width, height);

        SDL_Event event;
        SDL_WaitEvent(&event);

        if (HandleEvent(event))
            break;

        static auto diff = 0.01f;
        editor::global::windows_arr[0].splits_percentages[0] += diff;
        if (editor::global::windows_arr[0].splits_percentages[0] > 0.9f)
        {
            editor::global::windows_arr[0].splits_percentages[0] = 0.9f;
            diff *= -1;
        }
        else if (editor::global::windows_arr[0].splits_percentages[0] < 0.1f)
        {
            editor::global::windows_arr[0].splits_percentages[0] = 0.1f;
            diff *= -1;
        }

        // Move it to some event occurences or make a dirty-bit system.
        // TODO(Profiling): Redraw window only if something got dirty.
        RedrawWindow();

#ifdef DEBUG
        // Validate main window tree structure.
        ASSERT(Validate(editor::global::windows_arr + 1));
#endif

        // Some printing stuff:
        // DFS traverse the windows tree, and print the current window state:
#if 0
        {
            DEBUG_PrintWindowsState(global::windows_arr + 1);
            printf("\n");
        }
#endif
    }

    TTF_Quit();
    SDL_Quit();
    return 0;
}
