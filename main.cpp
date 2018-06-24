#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <SDL.h>
#include <SDL_ttf.h>

// #define RANDOM_WINDOW_COLORS

#ifdef RANDOM_WINDOW_COLORS
// TODO: Temporary.
#include <time.h>
#define WINDOW_COLOR (rand())
#else
#define WINDOW_COLOR (0xffffff)
#endif

// TODO: Move to other config file!
#define SUCCEEDED(expr_res) (static_cast<int>(expr_res) >= 0)
#define FAILED(expr_res) (static_cast<int>(expr_res) < 0)

// TODO: Refactor globals!
static SDL_Window *global_window;

// This is a surface that we put stuff on, and after that bit it to the screen.
// This uses SDL software rendering and there is no use of SLD_Renderer and
// stuff. After a resizing this pointer is changed. Do not free it!
static SDL_Surface *global_screen;

static int global_window_w, global_window_h;

static const int number_of_surfaces = 4;
static SDL_Surface *text_surface[number_of_surfaces];

// TODO: Differenciate vertical paddign from the top and from the bottom?
static int global_font_size = 15;

// Index of window that displays currently selected buffer.
static int global_current_window_idx;

#if 0
static int vertical_padding = 5;
static int horizontal_padding = 15;
static int first_item_height = 5;
#endif

enum WindowSplit { WIN_SPLIT_VERTCAL, WIN_SPLIT_HORIZONTAL };

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
};

// TODO: Is this too small limitation?
#define MAX_WINDOWS_ON_SPLIT (4)

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
};

// Window at index 0 is main window, created on init and it cannot be removed.
// TODO: Make these auto-resizable and think about initial size.
static int global_number_of_windows = 0;
static EditorWindow global_windows_arr[256];
static int global_number_of_buffers = 0;
static EditorBuffer global_buffers[256];

static EditorBuffer *CreateNewBuffer()
{
    global_buffers[global_number_of_buffers++] = { .color = WINDOW_COLOR };
    return global_buffers + global_number_of_buffers - 1;
}

static EditorWindow *CreateNewWindowWithBuffer(EditorBuffer *buffer,
                                               EditorWindow *root_window)
{
    global_windows_arr[global_number_of_windows++] = EditorWindow {
        .contains_buffer = 1,
        .buffer_ptr = buffer,
        .parent_ptr = root_window
    };

    return global_windows_arr + global_number_of_windows - 1;
}

// NOTE: This window must contain buffer!
// NOTE: This heavily assumes that this window is the active one.
// This focus is set to the buffer that is in the window before the split.
void EditorWindow::SplitWindow(WindowSplit split_type)
{
    if (contains_buffer)
    {
        EditorBuffer *previous_buffer_ptr = buffer_ptr;
        contains_buffer = 0;
        split = split_type;
        splits_percentages[0] = 0.5f;
        split_windows[0] = CreateNewWindowWithBuffer(previous_buffer_ptr, this);
        split_windows[1] = CreateNewWindowWithBuffer(CreateNewBuffer(), this);
        number_of_windows = 2;

        global_current_window_idx = split_windows[0] - global_windows_arr;
    }
    else
    {
        // TODO: Info, that something has gone horribly wrong...
        assert(false);
    }
}

static void InitializeFirstWindow()
{
    global_number_of_buffers = 2;
    global_buffers[0] = { .color = 0xffffff }; // Buffer with index 0 is a minibufer.
    global_buffers[1] = { .color = WINDOW_COLOR };

    global_number_of_windows = 4;

    // Window with index 0 contains minibuffer. And is drawn separetely.
    global_windows_arr[0] = EditorWindow {
        .contains_buffer = 1,
        .buffer_ptr = global_buffers,
        .parent_ptr = nullptr,
    };

    // Window with index 1 is main window.
    global_windows_arr[1] = EditorWindow {
        .contains_buffer = 1,
        .buffer_ptr = global_buffers + 1,
        .parent_ptr = nullptr
    };

    global_current_window_idx = 1;
}

static void DrawSplittingLine(const Rect &rect)
{
    SDL_Rect split_line = SDL_Rect { rect.x, rect.y, rect.width, rect.height };
    SDL_FillRect(global_screen, &split_line, 0x64645e);
}

static void DrawBufferOnRect(const EditorBuffer &buffer,
                             const Rect &rect,
                             const bool current_select)
{
    const SDL_Rect sdl_rect = { rect.x, rect.y, rect.width, rect.height };
    SDL_FillRect(global_screen, &sdl_rect, current_select ? 0xddee22 : buffer.color);

#if 0
    if (current_select)
    {
        const SDL_Rect selection_rect = {
            rect.x + 5, rect.y + 5,
            rect.width - 10, rect.height - 10
        };
        SDL_FillRect(global_screen, &selection_rect, 0x992211);
    }
#endif
}

static void DrawWindowOnRect(const EditorWindow &window,
                             const Rect &rect,
                             const bool is_selected)
{
    if (window.contains_buffer)
    {
        DrawBufferOnRect(* window.buffer_ptr, rect, is_selected);
    }
    else
    {
        // Tells at which pixel one of the windows in the split ends.
        // In range of (min_pos - max_pos);
        int split_idx[MAX_WINDOWS_ON_SPLIT];

        assert(window.number_of_windows >= 2);

        for (int i = 0; i < window.number_of_windows-1; ++i)
            assert(window.splits_percentages[i] > 0 &&
                   window.splits_percentages[i] < 1 &&
                   (i < window.number_of_windows-2
                    ? window.splits_percentages[i] < window.splits_percentages[i+1]
                    : 1));

        int min_pos = (window.split == WindowSplit::WIN_SPLIT_HORIZONTAL
                       ? rect.x : rect.y);
        int max_pos = (window.split == WindowSplit::WIN_SPLIT_HORIZONTAL
                       ? rect.x + rect.width : rect.y + rect.height);
        int difference = max_pos - min_pos;
        assert(difference > 0);

        for (int i = 0; i < window.number_of_windows-1; ++i)
        {
            split_idx[i] =
                min_pos + static_cast<int>(window.splits_percentages[i] * difference);
            // There is a one pixel space for splitting line.
            if (i > 0)
                split_idx[i] += 1;

            assert(min_pos < split_idx[i] && split_idx[i] < max_pos);
        }
        split_idx[window.number_of_windows-1] = max_pos;

        int previous_split = min_pos;
        for (int i = 0; i < window.number_of_windows; ++i)
        {
            Rect rect_to_draw_recusive_window =
                (window.split == WindowSplit::WIN_SPLIT_HORIZONTAL
                 ? Rect { previous_split, rect.y, split_idx[i]-previous_split, rect.height }
                 : Rect { rect.x, previous_split, rect.width, split_idx[i]-previous_split });

            // TODO: Move outside the loop for better performace?
            if (i < window.number_of_windows - 1)
            {
                Rect splitting_rect = (window.split == WindowSplit::WIN_SPLIT_HORIZONTAL
                     ? Rect { split_idx[i], rect.y, 1, rect.height }
                     : Rect { rect.x, split_idx[i], rect.width, 1 });

                DrawSplittingLine(splitting_rect);
            }

            // Add 1 becasue this is a space for the line separator.
            previous_split = split_idx[i]+1;
            DrawWindowOnRect(* window.split_windows[i],
                             rect_to_draw_recusive_window,
                             global_windows_arr + global_current_window_idx == window.split_windows[i]);
        }
    }
}

// TODO: Check if any of these functions can fail and handle this.
static int ResizeAndRedrawWindow()
{
    global_screen = SDL_GetWindowSurface(global_window);
    if (!global_screen)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "\e[91;1mCouldnt get the right surface of the window!\e[0m");
        assert(false);
    }

    SDL_GetWindowSize(global_window, &global_window_w, &global_window_h);

    // TODO(BUG): Splitting window to hard makes above function reporting that
    // the window size is 0.
    if (global_window_w == 0 || global_window_h == 0)
        assert(!"Size is 0");

    // Fill the whole screen, just in case.
    const SDL_Rect whole_screen = { 0, 0, global_window_w, global_window_h };
    SDL_FillRect(global_screen, &whole_screen, 0xff00ff); // Color suggesting error.

    assert(global_number_of_windows > 0);
    const EditorWindow main_window = global_windows_arr[1];
    const EditorWindow mini_buffer_window = global_windows_arr[0];

    DrawSplittingLine({ 0, global_window_h - 17, global_window_w, 1 });

    // Minibufer takes the fixed size of the window, so we draw it first, here:
    DrawWindowOnRect(mini_buffer_window,
                     Rect { 0, global_window_h - 17 +1, global_window_w, 17 },
                     global_current_window_idx == 0);

    DrawWindowOnRect(main_window,
                     Rect { 0, 0, global_window_w, global_window_h - 17 },
                     global_current_window_idx == 1);

    SDL_UpdateWindowSurface(global_window);

    return 0;
}

static int global_window_idx_before_entering_minibuffer;

// Set active first window to given root window.
static EditorWindow *GetFirstWindowInSubtree(EditorWindow *root_window)
{
    // If root window is a bufer, it is just selected.
    if (root_window->contains_buffer)
        return root_window;

    EditorWindow *next_window = root_window;
    while (!next_window->contains_buffer)
        next_window = next_window->split_windows[0];

    return next_window;
}

static EditorWindow *GetNextActiveWindow(const EditorWindow *current_window)
{
    EditorWindow *parent = current_window->parent_ptr;
    if (!parent)
    {
        return GetFirstWindowInSubtree(global_windows_arr + 1);
    }

    // Parent must be splited window.
    assert(!parent->contains_buffer);

    int index_in_parent = -1;
    for (int i = 0; i < parent->number_of_windows; ++i)
        if (parent->split_windows[i] == current_window)
        {
            index_in_parent = i;
            break;
        }

    assert(index_in_parent >= 0);
    if (index_in_parent + 1 >= parent->number_of_windows)
    {
        // Last window in this split - we must go to the upper window and search
        // the next one recusively.
        return GetFirstWindowInSubtree(GetNextActiveWindow(parent));
    }
    else
    {
        return parent->split_windows[index_in_parent + 1];
    }
}

static void SwitchToNextWidnow()
{
    if (global_current_window_idx == 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "\e[91;1mCannot switch window in minibuffer!\e[0m");
        return;
    }

    // TODO: Do we assume there is at least one buffer at a time?
    assert(global_number_of_buffers);

#if 0
    int next_window_idx = global_current_window_idx;
    do
    {
        next_window_idx++;
        if (next_window_idx >= global_number_of_windows)
            next_window_idx = 1;
    } while (!global_windows_arr[next_window_idx].contains_buffer
             && next_window_idx != global_current_window_idx);


    if (global_current_window_idx == next_window_idx)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "\e[93;1mWindow unchanged, because there is no other window!\e[0m");
    }
    global_current_window_idx = next_window_idx;
#else
    EditorWindow *next_window  =
        GetNextActiveWindow(global_windows_arr + global_current_window_idx);

    global_current_window_idx = next_window - global_windows_arr;
#endif
}

static void SwitchToMiniBuffer()
{
    if (global_current_window_idx == 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "\e[91;1mAlready in minibuffer\e[0m");
        return;
    }

    global_window_idx_before_entering_minibuffer
        = global_current_window_idx;
    global_current_window_idx = 0;
}

static void SwitchOutFromMiniBuffer()
{
    if (global_current_window_idx != 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "\e[91;1mNot in minibuffer\e[0m");
        return;
    }

    global_current_window_idx
        = global_window_idx_before_entering_minibuffer;
}

static int HandleEvent(const SDL_Event &event)
{
    int shouldQuit = 0;

    switch (event.type)
    {
        case SDL_QUIT:
        {
            SDL_Log("SDL_QUIT\n");
            shouldQuit = 1;
        } break;

        case SDL_KEYDOWN:
        {
#if 1
            // W - switch window.
            if (event.key.keysym.sym == 119)
            {
                SwitchToNextWidnow();
            }
            else if (event.key.keysym.sym == 120)
            {
                SwitchToMiniBuffer();
            }
            else if (event.key.keysym.sym == 103)
            {
                SwitchOutFromMiniBuffer();
            }
            else if (event.key.keysym.sym == 104)
            {
                if (global_current_window_idx != 0)
                {
                    global_windows_arr[global_current_window_idx].
                        SplitWindow(WindowSplit::WIN_SPLIT_HORIZONTAL);
                }
                else
                {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                                 "\e[93;1mCannot split minibuffer!\e[0m");
                }
            }
            else if (event.key.keysym.sym == 118)
            {
                if (global_current_window_idx != 0)
                {
                    global_windows_arr[global_current_window_idx].
                        SplitWindow(WindowSplit::WIN_SPLIT_VERTCAL);
                }
                else
                {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                                 "\e[93;1mCannot split minibuffer!\e[0m");
                }
            }

            if (global_windows_arr[global_current_window_idx].contains_buffer)
            {
                SDL_Log("Current window: %d (with color: %x)",
                        global_current_window_idx,
                        global_windows_arr[global_current_window_idx]
                            .buffer_ptr->color);
            }
#else
            SDL_Log("KEY DOWN %d\n", event.key.keysym.sym);
#endif
        } break;

        case SDL_KEYUP:
        {
            SDL_Log("KEY UP\n");
        } break;

        case SDL_WINDOWEVENT:
        {
            switch (event.window.event)
            {
                case SDL_WINDOWEVENT_SHOWN:
                {
                    SDL_Log("Window %d shown", event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_HIDDEN:
                {
                    SDL_Log("Window %d hidden", event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_EXPOSED:
                {
                    SDL_Log("Window %d exposed", event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_MOVED:
                {
                    SDL_Log("Window %d moved to %d,%d",
                            event.window.windowID,
                            event.window.data1,
                            event.window.data2);
                } break;

                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                {
                    SDL_Log("Window %d resized", event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_MINIMIZED:
                {
                    SDL_Log("Window %d minimized", event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_MAXIMIZED:
                {
                    SDL_Log("Window %d maximized", event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_RESTORED:
                {
                    SDL_Log("Window %d restored", event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_ENTER:
                {
                    SDL_Log("Mouse entered window %d", event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_LEAVE:
                {
                    SDL_Log("Mouse left window %d", event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_FOCUS_GAINED:
                {
                    SDL_Log("Window %d gained keyboard focus",
                            event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_FOCUS_LOST:
                {
                    SDL_Log("Window %d lost keyboard focus",
                            event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_CLOSE:
                {
                    SDL_Log("Window %d closed", event.window.windowID);
                } break;

#if SDL_VERSION_ATLEAST(2, 0, 5)
                case SDL_WINDOWEVENT_TAKE_FOCUS:
                {
                    SDL_Log("Window %d is offered a focus",
                            event.window.windowID);
                } break;

                case SDL_WINDOWEVENT_HIT_TEST:
                {
                    SDL_Log("Window %d has a special hit test",
                            event.window.windowID);
                } break;
#endif

                default:
                {
                    SDL_Log("Window %d got unknown event %d",
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
    // TODO: Possibly inicialize with SDL_INIT_EVENTS?
    if (FAILED(SDL_Init(SDL_INIT_VIDEO)))
    {
        // TODO: Logging.
        return -1;
    }

    if (FAILED(TTF_Init()))
    {
        // TODO: Logging.
        return -1;
    }

    return 0;
}

static int InitWindow(const int width, const int height)
{
    // NOTE(Testing): Sometimes it is good to set width and height to 0, 0 and
    // test what happens when window is smallest possible.
    global_window = SDL_CreateWindow("Editor",
                                     SDL_WINDOWPOS_UNDEFINED,
                                     SDL_WINDOWPOS_UNDEFINED,
                                     width, height, 0);
                                     // 0, 0, 0);

    if (!global_window)
    {
        // TODO: Logging.
        return -1;
    }

    // Instead of creating a renderer, draw directly to the screen.
    global_screen = SDL_GetWindowSurface(global_window);

    if (!global_screen)
    {
        // TODO: Logging.
        return -1;
    }

    return 0;
}

int main(void)
{
    if (FAILED(InitSDL()))
    {
        // TODO: Logging.
        exit(1);
    }

    // NOTE: Some system diagnostics.
    // Declare display mode structure to be filled in.
    SDL_DisplayMode current;

    if (SDL_GetCurrentDisplayMode(0, &current) != 0)
        SDL_Log("Could not get display mode for video display #%d: %s",
                0, SDL_GetError());
    else
        // On success, print the current display mode.
        SDL_Log("Display #%d: current display mode is %dx%dpx @ %dhz.",
                0, current.w, current.h, current.refresh_rate);

    // The size of the window to render.
    global_window_w = current.w / 3;
    global_window_h = current.h / 2;

    int window_x = current.w / 2 - global_window_w / 2,
        window_y = current.h / 2 - global_window_h / 2;

    // TODO: Use values based on screen width/height, or let user set his own.
    if (FAILED(InitWindow(global_window_w, global_window_h)))
    {
        // TODO: Logging.
        exit(1);
    }
    SDL_SetWindowPosition(global_window, window_x, window_y);


    InitializeFirstWindow();

    // TODO: Make it same as background color!
    const SDL_Rect foo = { 0, 0, global_window_w, global_window_h };
    SDL_FillRect(global_screen, &foo, 0x272822);

#if 0
    assert(global_bitmap_surface);
#endif

    TTF_Font *font_test =
        TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSansMono.ttf", global_font_size);

    if (!(text_surface[0] = TTF_RenderText_Solid(font_test,
                                                 "This text is: Solid",
                                                 (SDL_Color){0xF8, 0xF8, 0xF8, 255}))

        || !(text_surface[1] = TTF_RenderText_Shaded(font_test,
                                                     "This text is: Shaded",
                                                     (SDL_Color){0xF8, 0xF8, 0xF8, 255},
                                                     (SDL_Color){0x15, 0x15, 0x80, 0xFF}))

        || !(text_surface[2] = TTF_RenderText_Blended(font_test,
                                                      "Compilation started at Fri Jun",
                                                      (SDL_Color){0xF8, 0xF8, 0xF8, 255}))

        || !(text_surface[3] = TTF_RenderText_Blended(font_test,
                                                      "#define",
                                                      (SDL_Color){0xF9, 0x26, 0x72, 255})))
    {
        // TODO: handle error here, perhaps print TTF_GetError at least.
        assert(false);
    }

    TTF_CloseFont(font_test);
    font_test = nullptr;

    for (;;)
    {
        int width, height;
        SDL_GetWindowSize(global_window, &width, &height);
        SDL_Log("SIZE: %d x %d\n", width, height);

        SDL_Event event;
        SDL_WaitEvent(&event);

        if (HandleEvent(event))
            break;

        static float diff = 0.01f;
        global_windows_arr[0].splits_percentages[0] += diff;
        if (global_windows_arr[0].splits_percentages[0] > 0.9f)
        {
            global_windows_arr[0].splits_percentages[0] = 0.9f;
            diff *= -1;
        }
        else if (global_windows_arr[0].splits_percentages[0] < 0.1f)
        {
            global_windows_arr[0].splits_percentages[0] = 0.1f;
            diff *= -1;
        }

        // Move it to some event occurences or make a dirty-bit system.
        ResizeAndRedrawWindow();
    }

    TTF_Quit();
    SDL_Quit();
    return 0;
}
