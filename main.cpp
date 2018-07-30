#include <stdio.h>
#include <cstdint>

// TODO(Cleanup): Move to other config file!
#define SUCCEEDED(expr_res) (static_cast<int>(expr_res) >= 0)
#define FAILED(expr_res) (static_cast<int>(expr_res) < 0)
#define IS_NULL(expr_res) ((expr_res) == nullptr)

// TODO(Cleanup): Change/refactor or just add summary.
#define IS_EARLIER_IN_ARR(ARR, FIRST, SECOND) (FIRST - ARR < SECOND - ARR)

#define DEBUG
#define LOGGING
#define DG_LOG_LVL DG_LOG_LVL_WARN // DG_LOG_LVL_ALL
#define DG_USE_COLORS 1
#include "debug_goodies.h"

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#include <SDL.h>
#include <SDL_ttf.h>

#include "gap_buffer.cpp"

// #define RANDOM_WINDOW_COLORS
#ifdef RANDOM_WINDOW_COLORS
// TODO(Temporary):
#include <time.h>
#define WINDOW_COLOR (rand())
#else
#define WINDOW_COLOR (0x101010)
#endif

enum WindowSplit { WIN_SPLIT_VERTCAL, WIN_SPLIT_HORIZONTAL };
enum WindowTraverseMode { WIN_TRAVERSE_FORWARD, WIN_TRAVERSE_BACKWARDS };

struct Rect
{
    int x;
    int y;
    int width;
    int height;
};

struct EditorBuffer
{
    int color;

    gap_buffer one_line_buffer;
};

#define MAX_WINDOWS_ON_SPLIT (10)
#define MIN_PERCANTAGE_WINDOW_SPLIT (0.2f)

struct EditorWindow
{
    // If 0, window is splited to multiple windows, either vertically or
    // horizontally. Otherwise pointer to buffer, that is displayed in window is
    // stored.
    int contains_buffer;

    // A pointer to the splitting window, which owns this one, or
    // nullptr, if this is a top window. (If nullptr, there should be
    // only one window in the screen (expect minibuffer)).
    EditorWindow *parent_ptr;

    // This must be updated after every window resize.
    Rect position;

    union
    {
        struct // If contains single buffer:
        {
            EditorBuffer *buffer_ptr;
        };
        struct // If is splited into multiple windows:
        {
            WindowSplit split;
            // Must be < MAX_WINDOWS_ON_SPLI and >= 2
            int number_of_windows;
            // Splits percentages must be increasing and in range 0-1.
            float splits_percentages[MAX_WINDOWS_ON_SPLIT - 1];
            EditorWindow *split_windows[MAX_WINDOWS_ON_SPLIT];
        };
    };

    void SplitWindow(WindowSplit split_type);
    void UpdateSize(Rect new_rect);
    void Redraw(bool current_select) const;
    int GetIndexInParent() const;
    bool IsFirstInParent() const;
    bool IsLastInParent() const;

    int GetDimForSplitType(WindowSplit split_type) const;
    float GetScreenPercForSplitType(WindowSplit split_type) const;
};

// TODO(Cleanup): Try to use as less globals as possible...
namespace global
{
    // TODO(Platform): Make this platform-dependent.
    static SDL_Window *window;

    // This is a surface that we put stuff on, and after that bit it to the
    // screen. This uses SDL software rendering and there is no use of
    // SLD_Renderer and stuff. After a resizing this pointer is changed.
    // Do not free it!
    // TODO(Platform): Make this platform-dependent.
    static SDL_Surface *screen;

    static int window_w, window_h;

    static const int number_of_surfaces = 5;
    static SDL_Surface *alphabet['z' - 'a' + 1];
    static SDL_Surface *text_surface[number_of_surfaces];

    static const auto font_size = 13.f;

    // Window at index 0 is main window, created on init and it cannot be
    // removed. TODO(Testing): Make these auto-resizable and think about initial size.
    static int number_of_windows = 0;
    static EditorWindow windows_arr[256];
    static int number_of_buffers = 0;
    static EditorBuffer buffers[256];

    // Index of window that displays currently selected buffer.
    static int current_window_idx;

    // Index of active window before entering minibuffer.
    // TODO(Cleanup): Default value.
    static int window_idx_before_entering_minibuffer;
}

// TODO(Splitting lines): Remove unused atrribute!
// __attribute__ ((unused))
static void DrawSplittingLine(const Rect &rect)
{
    auto split_line = SDL_Rect { rect.x, rect.y, rect.width, rect.height };
    SDL_FillRect(global::screen, &split_line, 0x000000); // 0x64645e
}

static EditorBuffer *CreateNewBuffer()
{
    global::buffers[global::number_of_buffers].color = WINDOW_COLOR;
    global::buffers[global::number_of_buffers].one_line_buffer.initialize();

    auto result = global::buffers + global::number_of_buffers;
    global::number_of_buffers++;
    return result;
}

static EditorWindow *CreateNewWindowWithBuffer(EditorBuffer *buffer,
                                               EditorWindow *root_window)
{
    global::windows_arr[global::number_of_windows++] =
        EditorWindow
        {
            .contains_buffer = 1,
            .buffer_ptr = buffer,
            .parent_ptr = root_window
        };

    return global::windows_arr + global::number_of_windows - 1;
}

int EditorWindow::GetIndexInParent() const
{
    auto result = -1;
    for (auto i = 0; i < parent_ptr->number_of_windows; ++i)
        if (parent_ptr->split_windows[i] == this)
        {
            result = i;
            break;
        }
    ASSERT(result >= 0);
    return result;
}

int EditorWindow::GetDimForSplitType(WindowSplit split_type) const
{
    switch (split_type)
    {
        case WIN_SPLIT_VERTCAL:
            return position.height;
        case WIN_SPLIT_HORIZONTAL:
            return position.width;

        default:
            UNREACHABLE();
    }
}

float EditorWindow::GetScreenPercForSplitType(WindowSplit split_type) const
{
    switch (split_type)
    {
        case WIN_SPLIT_VERTCAL:
            return static_cast<float>(position.height) / global::window_h;
        case WIN_SPLIT_HORIZONTAL:
            return static_cast<float>(position.width) / global::window_w;

        default:
            UNREACHABLE();
    }
}

bool EditorWindow::IsFirstInParent() const
{
    return (parent_ptr->split_windows[0] == this);
}

bool EditorWindow::IsLastInParent() const
{
    return (parent_ptr->split_windows[parent_ptr->number_of_windows - 1] == this);
}

// NOTE: This window must contain buffer!
// NOTE: This heavily assumes that this window is the active one.
// This focus is set to the buffer that is in the window before the split.
void EditorWindow::SplitWindow(WindowSplit split_type)
{
    // We won't let user split the window, if one of the windows takes less than
    // the min size of the window.
    auto split_h_or_v = (split_type == WIN_SPLIT_VERTCAL
                         ? static_cast<float>(position.height) / global::window_h
                         : static_cast<float>(position.width) / global::window_w);

    if (split_h_or_v < MIN_PERCANTAGE_WINDOW_SPLIT * 2)
    {
        LOG_ERROR("Cannot split. The window is too small");
        return;
    }

    if (contains_buffer)
    {
        auto new_buffer = CreateNewBuffer();
        auto previous_buffer_ptr = buffer_ptr;

        // parent_ptr cannot contain buffer!
        ASSERT(!parent_ptr || !parent_ptr->contains_buffer);

        if (!parent_ptr || parent_ptr->split != split_type)
        {
            // We must create new window, which holds these two:
            split = split_type;
            splits_percentages[0] = 0.5f;
            splits_percentages[1] = 1.0f;
            split_windows[0] = CreateNewWindowWithBuffer(previous_buffer_ptr, this);
            split_windows[1] = CreateNewWindowWithBuffer(new_buffer, this);
            number_of_windows = 2;
            global::current_window_idx = split_windows[0] - global::windows_arr;
            contains_buffer = 0;
            UpdateSize(position);
        }
        else
        {
            // TODO(Cleanup): Make sure that this assertion won't fire.
            ASSERT(parent_ptr->number_of_windows < MAX_WINDOWS_ON_SPLIT - 1);

            auto idx_in_parent = GetIndexInParent();

            // Move windows with bigger index by one in the array.
            for (auto i = parent_ptr->number_of_windows;
                 i >= idx_in_parent + 1;
                 --i)
            {
                parent_ptr->split_windows[i] = parent_ptr->split_windows[i-1];
                parent_ptr->splits_percentages[i] = parent_ptr->splits_percentages[i-1];
            }
            parent_ptr->split_windows[idx_in_parent + 1] =
                CreateNewWindowWithBuffer(new_buffer, parent_ptr);

            // Calculate the split which is half between split percent under
            // index of THIS buffer, and the next one nad insert new window with
            // buffer 'new_buffer' there.
            auto previous_split = (idx_in_parent == 0
                                   ? 0
                                   : parent_ptr->splits_percentages[idx_in_parent - 1]);
            auto split_perct =
                (parent_ptr->splits_percentages[idx_in_parent] + previous_split) / 2;
            parent_ptr->splits_percentages[idx_in_parent] = split_perct;

            // We can safetly increment this, becasue of the check above.
            parent_ptr->number_of_windows++;

            // TODO(Cleanup): i think this should be false, because parent must
            // be split window (does not contain a single buffer), and therefore
            // cannot be selected.
            parent_ptr->UpdateSize(parent_ptr->position);
            parent_ptr->Redraw(false);
        }
    }
    else
        PANIC("This windows cannot be splited and should never ever be focused!");
}

// TODO(Cleanup): Summary
void EditorWindow::UpdateSize(Rect new_rect)
{
    position = new_rect;

    // If windows contains multiple windows, we must update their size first.
    if (!contains_buffer)
    {
        // Tells at which pixel one of the windows in the split ends.
        // In range of (min_pos - max_pos);
        int split_idx[MAX_WINDOWS_ON_SPLIT];

        ASSERT(number_of_windows >= 2);

        for (auto i = 0; i < number_of_windows-1; ++i)
            ASSERT(splits_percentages[i] > 0 &&
                   splits_percentages[i] < 1 &&
                   (i < number_of_windows-2
                    ? splits_percentages[i] < splits_percentages[i+1]
                    : 1));

        auto min_pos = (split == WindowSplit::WIN_SPLIT_HORIZONTAL
                        ? position.x : position.y);
        auto max_pos = (split == WindowSplit::WIN_SPLIT_HORIZONTAL
                        ? position.x + position.width : position.y + position.height);
        auto difference = max_pos - min_pos;
        ASSERT(difference > 0);

        for (auto i = 0; i < number_of_windows-1; ++i)
        {
            split_idx[i] =
                min_pos + static_cast<int>(splits_percentages[i] * difference);

            // TODO(Splitting lines): Mayby just handle them as part of the buffer?
            // There is a one pixel space for splitting line.
            if (i > 0)
                split_idx[i] += 1;

            ASSERT(min_pos < split_idx[i] && split_idx[i] < max_pos);
        }
        split_idx[number_of_windows-1] = max_pos;

        auto previous_split = min_pos;
        for (auto i = 0; i < number_of_windows; ++i)
        {
            auto nested_window_rect =
                (split == WindowSplit::WIN_SPLIT_HORIZONTAL
                 ? Rect { previous_split, position.y,
                        split_idx[i]-previous_split, position.height }
                 : Rect { position.x, previous_split,
                           position.width, split_idx[i]-previous_split });

#if 1
            // TODO(Profiling): Move outside the loop for better performace?
            if (i < number_of_windows - 1)
            {
                auto splitting_rect =
                    (split == WindowSplit::WIN_SPLIT_HORIZONTAL
                     ? Rect {
                        split_idx[i], nested_window_rect.y,
                            1, nested_window_rect.height
                            }
                     : Rect {
                        nested_window_rect.x, split_idx[i],
                            nested_window_rect.width, 1
                            });

                DrawSplittingLine(splitting_rect);
            }
#endif // 0

            // Add 1 becasue this is a space for the line separator.
            previous_split = split_idx[i]+1;
            split_windows[i]->UpdateSize(nested_window_rect);
        }
    }
}

void EditorWindow::Redraw(bool current_select) const
{
    if (contains_buffer)
    {
        auto sdl_rect = SDL_Rect {
            position.x, position.y,
            position.width, position.height
        };

        SDL_FillRect(global::screen,
                     &sdl_rect,
                     current_select ? 0x272822 : buffer_ptr->color);

    }
    else
        for (auto i = 0; i < number_of_windows; ++i)
        {
            split_windows[i]->Redraw(global::current_window_idx ==
                                     (split_windows[i] - global::windows_arr));
        }
}

namespace editor_window_utility
{
    namespace detail
    {
        static void ResizeIthWindowLeft(EditorWindow* parent_window,
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

            // This does not work, because nester windows spoit them a lot.
#if 0
            if (parent_window->split_windows[index_in_parent - 1]
                ->GetScreenPercForSplitType(WindowSplit::WIN_SPLIT_HORIZONTAL) >= 0.2f)
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

        static void ResizeIthWindowRight(EditorWindow* parent_window,
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

    // TODO: Find better name.
    // [curr_window] is heavily assumed to be the selected window.
    static void ResizeWindowAux(EditorWindow* curr_window,
                                WindowSplit split_type,
                                WindowTraverseMode direction)
    {
        // TODO(Cleaup): Explain.
        auto right_parent = curr_window->parent_ptr;
        auto child = curr_window;
        while(right_parent
              && (right_parent->split != split_type
                  || (direction == WindowTraverseMode::WIN_TRAVERSE_BACKWARDS
                      &&  child->IsFirstInParent())
                  || (direction == WindowTraverseMode::WIN_TRAVERSE_FORWARD
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
            case (WindowTraverseMode::WIN_TRAVERSE_BACKWARDS):
                detail::ResizeIthWindowLeft(right_parent, index_in_parent);
                break;
            case (WindowTraverseMode::WIN_TRAVERSE_FORWARD):
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
    global::number_of_buffers = 2;
    global::buffers[0] = { .color = 0xffffff }; // Buffer with index 0 is a
    // minibufer.
    global::buffers[1] = { .color = WINDOW_COLOR };

    global::number_of_windows = 2;

    // Window with index 0 contains minibuffer. And is drawn separetely.
    global::windows_arr[0] = EditorWindow {
        .contains_buffer = 1,
        .buffer_ptr = global::buffers,
        .parent_ptr = nullptr,
        .position = Rect { 0, global::window_h - 17 +1, global::window_w, 17 }
    };

    // Window with index 1 is main window.
    global::windows_arr[1] = EditorWindow {
        .contains_buffer = 1,
        .buffer_ptr = global::buffers + 1,
        .parent_ptr = nullptr,
        .position = Rect { 0, 0, global::window_w, global::window_h - 17 }
    };

    global::current_window_idx = 1;
}

// TODO(Cleanup): Create namespace application and move resize and redraw window
//                functions and all globals there!
// TODO(Cleanup): Check if something here can fail,
//                if not change the type to void.
static int ResizeWindow()
{
    global::windows_arr[0].UpdateSize(Rect {
            0, global::window_h - 17 +1,
                global::window_w, 17
                });

    global::windows_arr[1].UpdateSize(Rect {
            0, 0,
                global::window_w, global::window_h - 17
                });

    return 0;
}

static void InitTextGlobalSutff()
{
    auto font_test = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSansMono.ttf",
                                  global::font_size);

    if (!(global::text_surface[0] =
          TTF_RenderText_Solid(font_test,
                               "This text is solid.",
                               SDL_Color {0xF8, 0xF8, 0xF8, 0xFF}))
        || !(global::text_surface[1] =
             TTF_RenderText_Shaded(font_test,
                                   "This text is shaded.",
                                   (SDL_Color){0xF8, 0xF8, 0xF8, 0xFF},
                                   // SDL_Color {0x15, 0x15, 0x80, 0xFF})) // 0x272822
                                   SDL_Color { 0x27, 0x28, 0x22, 0xFF}))
        || !(global::text_surface[2] =
             TTF_RenderText_Blended(font_test,
                                    "This text is smoothed.",
                                    SDL_Color {0xF8, 0xF8, 0xF8, 0xFF}))
        || !(global::text_surface[3] =
             TTF_RenderText_Blended(font_test,
                                    "This text is colored.",
                                    SDL_Color {0xF9, 0x26, 0x72, 0xFF}))
        || !(global::text_surface[4] =
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
        if (IS_NULL(global::alphabet[c] =
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

static void PrintTextLine(EditorWindow const* window_ptr,
                          int line_nr, // First visible line of the buffer is 0.
                          char const* text,
                          int cursor_idx) // gap_buffer const* line
{
    for (auto i = 0; text[i]; ++i)
    {
        auto text_idx = static_cast<int>(text[i]);
        auto draw_rect = SDL_Rect {
            // Only for monospace fonts.
            window_ptr->position.x + 2 + global::alphabet[text_idx]->w * i,
            window_ptr->position.y + 2 + global::text_surface[0]->h * line_nr,
            global::alphabet[text_idx]->w,
            global::alphabet[text_idx]->h
        };

        if (draw_rect.x + draw_rect.w >
            window_ptr->position.x + window_ptr->position.width
            || draw_rect.y + draw_rect.h >
            window_ptr->position.y + window_ptr->position.height)
        {
            return;
        }

        if (FAILED(SDL_BlitSurface(global::alphabet[text_idx], nullptr,
                                   global::screen, &draw_rect)))
        {
            PANIC("Bitting surface failed!");
        }
    }

    if (cursor_idx >= 0)
    {
        auto rect = SDL_Rect {
            window_ptr->position.x + 2 + global::alphabet[static_cast<int>('a')]->w * cursor_idx,
            window_ptr->position.y + 2 + global::text_surface[0]->h * line_nr,
            2,
            global::alphabet[static_cast<int>('a')]->h
        };

        SDL_FillRect(global::screen, &rect, 0xF8F8F8FF);
    }
}

static void PrintTextLineFromGapBuffer(EditorWindow const* window_ptr,
                                       int line_nr, // First visible line of the buffer is 0.
                                       gap_buffer const* line)
{
    auto text = line->to_c_str();
    auto idx = line->get_idx();

    PrintTextLine(window_ptr, line_nr, reinterpret_cast<char const*>(text), idx);

#if 0
    system("clear");
    printf("IDX: %d\n", idx);
    line->DEBUG_print_state();
#endif
    std::free(reinterpret_cast<void *>(const_cast<unsigned char*>(text)));
}

// TODO(Cleanup): Check if something here can fail, if not change the type to
// void.
static int RedrawWindow()
{
    global::screen = SDL_GetWindowSurface(global::window);
    if (!global::screen)
        PANIC("Couldnt get the right surface of the window!");

    SDL_GetWindowSize(global::window,
                      &global::window_w,
                      &global::window_h);

    // This is probobly obsolete and should be removed. This bug was fixed long
    // time ago. This used to happen when too many windows were created due to
    // the stack smashing.
    if (global::window_w == 0 || global::window_h == 0)
        PANIC("Size is 0!");

    ASSERT(global::number_of_windows > 0);
    auto main_window = (global::windows_arr + 1);
    auto mini_buffer_window = (global::windows_arr + 0);

    // TODO(Splitting lines): Decide how i want to draw them.
#if 1
    DrawSplittingLine({ 0, global::window_h - 17, global::window_w, 1 });
#endif

    mini_buffer_window->Redraw(global::current_window_idx == 0);
    main_window->Redraw(global::current_window_idx == 1);

    if (!(global::windows_arr + 1)->contains_buffer)
    {
        if ((global::windows_arr + 2)->contains_buffer
            && (global::windows_arr + 4)->contains_buffer
            && (global::windows_arr + 5)->contains_buffer)
        {
            auto left_w = global::windows_arr + 2;
            auto right_w = global::windows_arr + 4;
            auto gap_w = global::windows_arr + 5;

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

            PrintTextLineFromGapBuffer(gap_w, 0, &gap_w->buffer_ptr->one_line_buffer);
        }
    }

    SDL_UpdateWindowSurface(global::window);
    return 0;
}

// If traverse is forward, we pick the first window in the subtree of the
// root_window, otherwise we pick the last one (we always go either left or
// right recusively).
static EditorWindow *GetFirstOrLastWindowInSubtree(EditorWindow* root_window,
                                                   WindowTraverseMode traverse)
{
    // If root window is a bufer, it is just selected.
    if (root_window->contains_buffer)
        return root_window;

    auto next_window = root_window;
    auto window_to_pick_idx = 0;

    if (traverse == WindowTraverseMode::WIN_TRAVERSE_FORWARD)
        window_to_pick_idx = 0;
    else
    {
        ASSERT (traverse == WindowTraverseMode::WIN_TRAVERSE_BACKWARDS);
        window_to_pick_idx = next_window->number_of_windows - 1;
    }

    while (!next_window->contains_buffer)
        next_window = next_window->split_windows[window_to_pick_idx];

    return next_window;
}

static EditorWindow *GetNextOrPrevActiveWindow(const EditorWindow *current_window,
                                               const WindowTraverseMode traverse)
{
    auto parent = current_window->parent_ptr;
    if (!parent)
        return GetFirstOrLastWindowInSubtree(global::windows_arr + 1, traverse);

    // Parent must be splited window.
    ASSERT(!parent->contains_buffer);
    auto idx_in_parent = current_window->GetIndexInParent();

    // TODO(Cleanup): Compress this better!
    if (traverse == WindowTraverseMode::WIN_TRAVERSE_FORWARD)
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
        ASSERT(traverse == WindowTraverseMode::WIN_TRAVERSE_BACKWARDS);
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
static void SwitchWindow(const WindowTraverseMode traverse)
{
    if (global::current_window_idx == 0)
    {
        LOG_WARN("Cannot switch window in minibuffer!");
        return;
    }

    // TODO(Cleanup): Do we assume there is at least one buffer at a time?
    // TODO(Cleanup): What if there is just minibuffer?
    ASSERT(global::number_of_buffers);

    auto next_window =
        GetNextOrPrevActiveWindow(global::windows_arr + global::current_window_idx,
                                  traverse);
    ASSERT(next_window);
    global::current_window_idx = next_window - global::windows_arr;
}

static void SwitchToMiniBuffer()
{
    if (global::current_window_idx == 0)
    {
        LOG_WARN("Already in minibuffer");
        return;
    }

    global::window_idx_before_entering_minibuffer = global::current_window_idx;
    global::current_window_idx = 0;
}

static void SwitchOutFromMiniBuffer()
{
    if (global::current_window_idx != 0)
    {
        LOG_WARN("Not in minibuffer");
        return;
    }

    global::current_window_idx
        = global::window_idx_before_entering_minibuffer;
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
                SwitchWindow(WindowTraverseMode::WIN_TRAVERSE_FORWARD);
            }
            // Q - Switch window backwards.
            else if (event.key.keysym.sym == 113)
            {
                SwitchWindow(WindowTraverseMode::WIN_TRAVERSE_BACKWARDS);
            }

            // [<-] - Move left current window border.
            else if (event.key.keysym.sym == 1073741904)
            {
                auto curr_window = (global::windows_arr + global::current_window_idx);
                editor_window_utility
                    ::ResizeWindowAux(curr_window,
                                      WindowSplit::WIN_SPLIT_HORIZONTAL,
                                      WindowTraverseMode::WIN_TRAVERSE_BACKWARDS);
            }

            // [->] - Move right current window border.
            else if (event.key.keysym.sym == 1073741903)
            {
                auto curr_window = (global::windows_arr + global::current_window_idx);
                editor_window_utility
                    ::ResizeWindowAux(curr_window,
                                      WindowSplit::WIN_SPLIT_HORIZONTAL,
                                      WindowTraverseMode::WIN_TRAVERSE_FORWARD);
            }

            // [v] - Move down current window border.
            else if (event.key.keysym.sym == 1073741905)
            {
                auto curr_window = (global::windows_arr + global::current_window_idx);
                editor_window_utility
                    ::ResizeWindowAux(curr_window,
                                      WindowSplit::WIN_SPLIT_VERTCAL,
                                      WindowTraverseMode::WIN_TRAVERSE_FORWARD);
            }

            // [^] - Move up current window border.
            else if (event.key.keysym.sym == 1073741906)
            {
                auto curr_window = (global::windows_arr + global::current_window_idx);
                editor_window_utility
                    ::ResizeWindowAux(curr_window,
                                      WindowSplit::WIN_SPLIT_VERTCAL,
                                      WindowTraverseMode::WIN_TRAVERSE_BACKWARDS);
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
                        SplitWindow(WindowSplit::WIN_SPLIT_HORIZONTAL);
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
                        SplitWindow(WindowSplit::WIN_SPLIT_VERTCAL);
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
                (global::windows_arr + 5)->buffer_ptr
                                         ->one_line_buffer.insert_at_point(character);
                succeeded = true; // Inserting character cannot fail.
            }
            else if (arrow == 1)
                succeeded = (global::windows_arr + 5)->buffer_ptr
                                                     ->one_line_buffer.cursor_forward();
            else if (arrow == 2)
                succeeded = (global::windows_arr + 5)->buffer_ptr
                                                     ->one_line_buffer.cursor_backward();
            else if (backspace)
                succeeded = (global::windows_arr + 5)->buffer_ptr
                                                     ->one_line_buffer.delete_char_backward();
            else if (del)
                succeeded = (global::windows_arr + 5)->buffer_ptr
                                                     ->one_line_buffer.delete_char_forward();

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
    global::window = SDL_CreateWindow("Editor",
                                      SDL_WINDOWPOS_UNDEFINED,
                                      SDL_WINDOWPOS_UNDEFINED,
                                      width, height, 0);
                                      // 0, 0, 0); // To test corner-cases.

    if (!global::window)
    {
        // TODO(Debug): Logging.
        return -1;
    }

    // Instead of creating a renderer, draw directly to the screen.
    global::screen = SDL_GetWindowSurface(global::window);

    if (!global::screen)
    {
        // TODO(Debug): Logging.
        return -1;
    }

    return 0;
}

// Makes sure that the main window tree is done correctly. Inside horizontal
// split window cannot be another horizontal etc...
static bool Validate(EditorWindow *window)
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

static void DEBUG_PrintWindowsState(const EditorWindow *window)
{
    if (!window)
        printf(" (nullptr) ");
    else if (window->contains_buffer)
        printf(" (Buffer: %ld) ", window->buffer_ptr - global::buffers);
    else
    {
        printf(" (%c){ ", (window->split == WindowSplit::WIN_SPLIT_HORIZONTAL ? 'H' : 'V'));
        for(auto i = 0; i < window->number_of_windows; ++i)
            DEBUG_PrintWindowsState(window->split_windows[i]);
        printf("} ");
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
    global::window_w = current.w / 3;
    global::window_h = current.h / 2;

    auto window_x = current.w / 2 - global::window_w / 2;
    auto window_y = current.h / 2 - global::window_h / 2;

    // TODO(Platform): Use values based on screen width/height, or let user set
    // his own.
    if (FAILED(InitWindow(global::window_w, global::window_h)))
    {
        // TODO(Debug): Logging.
        exit(1);
    }
    SDL_SetWindowPosition(global::window, window_x, window_y);

    InitializeFirstWindow();

#if 1
    global::windows_arr[global::current_window_idx]
        .SplitWindow(WindowSplit::WIN_SPLIT_HORIZONTAL);
    SwitchWindow(WindowTraverseMode::WIN_TRAVERSE_FORWARD);
    global::windows_arr[global::current_window_idx]
        .SplitWindow(WindowSplit::WIN_SPLIT_VERTCAL);
    SwitchWindow(WindowTraverseMode::WIN_TRAVERSE_FORWARD);
#endif

    InitTextGlobalSutff();

    for (;;)
    {
        auto width = 0;
        auto height = 0;
        SDL_GetWindowSize(global::window, &width, &height);
        LOG_INFO("Current window size: %d x %d", width, height);

        SDL_Event event;
        SDL_WaitEvent(&event);

        if (HandleEvent(event))
            break;

        static auto diff = 0.01f;
        global::windows_arr[0].splits_percentages[0] += diff;
        if (global::windows_arr[0].splits_percentages[0] > 0.9f)
        {
            global::windows_arr[0].splits_percentages[0] = 0.9f;
            diff *= -1;
        }
        else if (global::windows_arr[0].splits_percentages[0] < 0.1f)
        {
            global::windows_arr[0].splits_percentages[0] = 0.1f;
            diff *= -1;
        }

        // Move it to some event occurences or make a dirty-bit system.
        // TODO(Profiling): Redraw window only if something got dirty.
        RedrawWindow();

#ifdef DEBUG
        // Validate main window tree structure.
        ASSERT(Validate(global::windows_arr + 1));
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
