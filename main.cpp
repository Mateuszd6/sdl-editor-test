#include "config.h"

// TODO(Cleanup): remove from here as many as possible.
#include "text/gap_buffer.hpp"
#include "text/undo_buffer.hpp"

#include "editor/buffer.hpp"
#include "editor/window.hpp"
#include "graphics/graphics.hpp"
#include "platfrom/platform.hpp"

// TODO(Cleaup): Only for unity build.
#include "text/gap_buffer.cpp"
#include "text/undo_buffer.cpp"
#include "editor/buffer.cpp"
#include "editor/window.cpp"
#include "graphics/graphics.cpp"
#include "platfrom/platform.cpp"

namespace editor_window_utility::detail
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

#if 0 // This does not work, because nested windows spoil them a lot.
        if (parent_window->split_windows[index_in_parent - 1]
            ->GetScreenPercForSplitType(window_split::WIN_SPLIT_HORIZONTAL) >= 0.2f)
#endif

        if (parent_window->splits_percentages[index_in_parent - 1] - prev_split
            >= MIN_PERCANTAGE_WINDOW_SPLIT)
        {
            parent_window->splits_percentages[index_in_parent - 1] -= 0.01f;
            parent_window->UpdateSize(parent_window->position);
            parent_window->redraw(false); // Split window, cannot select.
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
            parent_window->redraw(false);
        }
        else
            LOG_WARN("Right window is too small!!");
    }
}


namespace editor_window_utility
{
    // NOTE: [curr_window] is heavily assumed to be the selected window.
    // TODO: Find better name.
    // TODO: Remove unused atrribute!
    __attribute__ ((unused))
    static void ResizeWindowAux(::editor::window* curr_window,
                                ::editor::window_split split_type,
                                ::editor::window_traverse_mode direction)
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
            case (::editor::window_traverse_mode::WIN_TRAVERSE_BACKWARDS):
                detail::ResizeIthWindowLeft(right_parent, index_in_parent);
                break;
            case (::editor::window_traverse_mode::WIN_TRAVERSE_FORWARD):
                detail::ResizeIthWindowRight(right_parent, index_in_parent);
                break;

            default:
                UNREACHABLE();
        }
    }
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
    ::platform::initialize_font("/usr/share/fonts/TTF/DejaVuSansMono.ttf");
    ::platform::run_font_test();
    ::platform::destroy_font();
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
            auto home = false;
            auto buffer_start = false;
            auto end = false;
            auto buffer_end = false;
            auto enter = false;
            auto tab = false;
            auto page_up = false;
            auto page_down = false;

            // Generic test operation.
            auto test_operation = false;

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
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? ')' : '0';
                    break;
                case SDLK_1:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? '!' : '1';
                    break;
                case SDLK_2:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? '@' : '2';
                    break;
                case SDLK_3:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? '#' : '3';
                    break;
                case SDLK_4:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? '$' : '4';
                    break;
                case SDLK_5:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? '%' : '5';
                    break;
                case SDLK_6:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? '^' : '6';
                    break;
                case SDLK_7:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? '&' : '7';
                    break;
                case SDLK_8:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? '*' : '8';
                    break;
                case SDLK_9:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? '(' : '9';
                    break;
                case SDLK_SEMICOLON:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? ':' : ';';
                    break;
                case SDLK_QUOTE:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? '\"' : '\'';
                    break;
                case SDLK_RIGHTBRACKET:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? '}' : ']';
                    break;
                case SDLK_LEFTBRACKET:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? '{' : '[';
                    break;
                case SDLK_MINUS:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? '_' : '-';
                    break;
                case SDLK_EQUALS:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? '+' : '=';
                    break;
                case SDLK_PERIOD:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? '>' : '.';
                    break;
                case SDLK_COMMA:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? '<' : ',';
                    break;
                case SDLK_SLASH:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? '?' : '/';
                    break;
                case SDLK_BACKSLASH:
                    character = (event.key.keysym.mod & KMOD_SHIFT) ? '|' : '\\';
                    break;

                case SDLK_TAB:
                    tab = true;
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

                case SDLK_PAGEDOWN:
                    page_down = true;
                    break;

                case SDLK_PAGEUP:
                    page_up = true;
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

                case SDLK_HOME:
                {
                    if (event.key.keysym.mod & KMOD_LCTRL)
                        buffer_start = true;
                    else
                        home = true;
                } break;

                case SDLK_END:
                {
                    if (event.key.keysym.mod & KMOD_LCTRL)
                        buffer_end = true;
                    else
                        end = true;
                } break;

                case SDLK_ESCAPE:
                    test_operation = true;
                    break;

                case SDLK_RETURN:
                    enter = true;
                    break;

                default:
                    break;
            }

            auto current_window = (editor::global::windows_arr + 1);
#endif

#if 1
            if(character != '\0')
            {
                current_window->buf_point.insert_character_at_point(character);
            }

            else if(tab)
            {
                for(auto i = 0; i < 4; ++i)
                    current_window->buf_point.insert_character_at_point(' ');
            }

            else if(backspace)
            {
                if (!current_window->buf_point.remove_character_backward())
                    LOG_WARN("Could not remove character backwards!");
            }

            else if(page_up)
            {
                auto number_of_displayed_lines = static_cast<uint64>(
                    (current_window->position.height - 2) / ::platform::get_letter_height());
                if (!current_window->buf_point.jump_up(number_of_displayed_lines))
                    LOG_WARN("Cannot move up!");
            }

            else if(page_down)
            {
                auto number_of_displayed_lines = static_cast<uint64>(
                    (current_window->position.height - 2) / ::platform::get_letter_height());
                if (!current_window->buf_point.jump_down(number_of_displayed_lines))
                    LOG_WARN("Cannot move down!");
            }

            else if (del)
            {
                if (!current_window->buf_point.remove_character_forward())
                    LOG_WARN("Could not remove character forward!");
            }

            else if (enter)
            {
                current_window->buf_point.insert_newline_at_point();
            }

            else if(arrow == 1)
            {
                if (!current_window->buf_point.character_right())
                    LOG_WARN("Cannot move right. No next character.");
            }

            else if(arrow == 2)
            {
                if (!current_window->buf_point.character_left())
                    LOG_WARN("Cannot move left. No previous character.");
            }

            else if(arrow == 3)
            {
                if (!current_window->buf_point.line_up())
                    LOG_WARN("Cannot move up. No previous line.");
            }

            else if(arrow == 4)
            {
                if (!current_window->buf_point.line_down())
                    LOG_WARN("Cannot move down. No next line.");
            }

            else if(home)
            {
                current_window->buf_point.line_start();
            }

            else if(buffer_start)
            {
                current_window->buf_point.buffer_start();
            }

            else if(end)
            {
                current_window->buf_point.line_end();
            }

            else if(buffer_end)
            {
                current_window->buf_point.buffer_end();
            }

            else if(test_operation)
            {
                current_window->buf_point.buffer_ptr
                        ->get_line(current_window->buf_point.curr_line)
                        ->insert_at_point(current_window->buf_point.curr_idx++, '0');

                // Insert newline
                {
                    current_window->buf_point.buffer_ptr
                        ->insert_newline_correct(current_window->buf_point.curr_line, current_window->buf_point.curr_idx);

                    current_window->buf_point.curr_line++;
                    current_window->buf_point.curr_idx = 0;
                }

                for(auto i = 80; i > 0; --i)
                {
                    auto m = i;
                    while(m)
                    {
                        current_window->buf_point.buffer_ptr
                            ->get_line(current_window->buf_point.curr_line)
                            ->insert_at_point(current_window->buf_point.curr_idx++, '0' + (m % 10));
                        current_window->buf_point.curr_idx = 0;
                        m /= 10;
                    }

                    // Move to the beginning of prev. line
                    {
                        current_window->buf_point.curr_line--;
                        current_window->buf_point.curr_idx = current_window
                            ->buf_point.buffer_ptr
                            ->get_line(current_window->buf_point.curr_line)
                            ->size();
                    }

                    // Insert newline
                    {
                        current_window->buf_point.buffer_ptr
                            ->insert_newline_correct(current_window->buf_point.curr_line,
                                                     current_window->buf_point.curr_idx);

                        current_window->buf_point.curr_line++;
                        current_window->buf_point.curr_idx = 0;
                    }
                }
            }


            current_window->buf_point.buffer_ptr->undo.DEBUG_print_state();
#if 0
            printf("Line: %ld\nIndex: %ld",
                   current_window->buf_point.curr_line,
                   current_window->buf_point.curr_idx);
            current_window->buf_point.buffer_ptr->DEBUG_print_state();
#endif
#endif
#if 0
            if(character != '\0')
                current_window->buf_point.insert_character(character);
            else if(arrow == 1)
                current_window->buf_point.move_forward_curr_line();
            else if(arrow == 2)
                current_window->buf_point.move_backward_curr_line();
            else if(arrow == 3)
                LOG_WARN("Moved line up: %d", current_window->buf_point.move_line_up());
            else if(arrow == 4)
                LOG_WARN("Moved line down: %d", current_window->buf_point.move_line_down());
            else if (enter)
                current_window->buf_point.insert_newline();
            else if(backspace)
                current_window->buf_point.delete_char_at_cursor_backward();
            else if(del)
                current_window->buf_point.delete_char_at_cursor_forward();
            else if (tab)
                for (int i = 0_u8; i < 4_u8; ++i)
                    current_window->buf_point.insert_character(' ');

            current_window->buf_point.buffer_ptr->DEBUG_print_state();
#endif
#if 0

            else if(home)
                current_window->buffer_ptr->chunks[0].jump_start_line();
            else if(end)
                current_window->buffer_ptr->chunks[0].jump_end_line();
            else if(test_operation)
                for (int j = 0; j < 7; ++j)
                    for (int i = 0; i < 10; ++i)
                        current_window->buffer_ptr->insert_character('0' + i);

            if (!succeeded)
                LOG_WARN("Could not make the operation!");


            LOG_TRACE("GAP size of the current line: %lu",
                      current_window
                      ->buffer_ptr
                      ->lines[current_window->buffer_ptr->curr_line]
                      .get_gap_size());
#endif
        } break;

        case SDL_KEYUP:
        {
#if 0
            LOG_INFO("MSG ---> KEY UP %d", event.key.keysym.sym);
#endif
        } break;

        case SDL_WINDOWEVENT:
        {
            switch (event.window.event)
            {
                case SDL_WINDOWEVENT_SHOWN:
                {
#if 0
                    LOG_INFO("MSG ---> Window %d shown", event.window.windowID);
#endif
                } break;

                case SDL_WINDOWEVENT_HIDDEN:
                {
#if 0
                    LOG_INFO("MSG ---> Window %d hidden", event.window.windowID);
#endif
                } break;

                case SDL_WINDOWEVENT_EXPOSED:
                {
#if 0
                    LOG_INFO("MSG ---> Window %d exposed", event.window.windowID);
#endif
                } break;

                case SDL_WINDOWEVENT_MOVED:
                {
#if 0
                    LOG_INFO("MSG ---> Window %d moved to %d,%d",
                             event.window.windowID,
                             event.window.data1,
                             event.window.data2);
#endif
                } break;

                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                {
#if 0
                    LOG_INFO("MSG ---> Window %d resized", event.window.windowID);
#endif
                    ResizeWindow();
                } break;

                case SDL_WINDOWEVENT_MINIMIZED:
                {
#if 0
                    LOG_INFO("MSG ---> Window %d minimized", event.window.windowID);
#endif
                } break;

                case SDL_WINDOWEVENT_MAXIMIZED:
                {
#if 0
                    LOG_INFO("MSG ---> Window %d maximized", event.window.windowID);
#endif
                } break;

                case SDL_WINDOWEVENT_RESTORED:
                {
#if 0
                    LOG_INFO("MSG ---> Window %d restored", event.window.windowID);
#endif
                } break;

                case SDL_WINDOWEVENT_ENTER:
                {
#if 0
                    LOG_INFO("MSG ---> Mouse entered window %d", event.window.windowID);
#endif
                } break;

                case SDL_WINDOWEVENT_LEAVE:
                {
#if 0
                    LOG_INFO("MSG ---> Mouse left window %d", event.window.windowID);
#endif
                } break;

                case SDL_WINDOWEVENT_FOCUS_GAINED:
                {
#if 0
                    LOG_INFO("MSG ---> Window %d gained keyboard focus", event.window.windowID);
#endif
                } break;

                case SDL_WINDOWEVENT_FOCUS_LOST:
                {
#if 0
                    LOG_INFO("MSG ---> Window %d lost keyboard focus", event.window.windowID);
#endif
                } break;

                case SDL_WINDOWEVENT_CLOSE:
                {
#if 0
                    LOG_INFO("MSG ---> Window %d closed", event.window.windowID);
#endif
                } break;

#if SDL_VERSION_ATLEAST(2, 0, 5)
                case SDL_WINDOWEVENT_TAKE_FOCUS:
                {
#if 0
                    LOG_INFO("MSG ---> Window %d is offered a focus", event.window.windowID);
#endif
                } break;

                case SDL_WINDOWEVENT_HIT_TEST:
                {
#if 0
                    LOG_INFO("MSG ---> Window %d has a special hit test", event.window.windowID);
#endif
                } break;
#endif

                default:
                {
#if 0
                    LOG_INFO("MSG ---> Window %d got unknown event %d",
                             event.window.windowID,
                             event.window.event);
#endif
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
static bool Validate(::editor::window *window)
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

    ::editor::initialize_first_window();

#if 0
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

#if 0
        LOG_INFO("Current window size: %d x %d", width, height);
#endif

        SDL_Event event;
        SDL_WaitEvent(&event);

#if 0 // TODO(Cleaup): I think this is obsolete stuff, but check it out.
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
#endif

        if (HandleEvent(event))
            break;

        // Move it to some event occurences or make a dirty-bit system.
        // TODO(Profiling): Redraw window only if something got dirty.
        ::platform::redraw_window();

#ifdef DEBUG
        // Validate main window tree structure.
        ASSERT(Validate(::editor::global::windows_arr + 1));
#endif

#if 0
        // system("clear");
        (::editor::global::windows_arr + ::editor::global::current_window_idx)
            ->buf_point
            .curr_chunk
            ->DEBUG_print_state();
#endif
    }

    TTF_Quit();
    SDL_Quit();
    return 0;
}
