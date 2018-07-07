#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <SDL.h>
#include <SDL_ttf.h>

#define RANDOM_WINDOW_COLORS

#ifdef RANDOM_WINDOW_COLORS
// TODO(Temporary):
#include <time.h>
#define WINDOW_COLOR (rand())
#else
#define WINDOW_COLOR (0xffffff)
#endif

// TODO(Cleanup): Move to other config file!
#define SUCCEEDED(expr_res) (static_cast<int>(expr_res) >= 0)
#define FAILED(expr_res) (static_cast<int>(expr_res) < 0)

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

#define MAX_WINDOWS_ON_SPLIT (4)

struct EditorWindow
{
    // Heavily assumes, that there is only one window currently selected.
    int is_selected;

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
};

// TODO(Cleanup): Try to use as less globals as possible...
namespace globals
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

    static const int number_of_surfaces = 4;
    static SDL_Surface *text_surface[number_of_surfaces];

    static int font_size = 15;

    // Window at index 0 is main window, created on init and it cannot be
    // removed. TODO(Testing): Make these auto-resizable and think about initial size.
    static int number_of_windows = 0;
    static EditorWindow windows_arr[256];
    static int number_of_buffers = 0;
    static EditorBuffer buffers[256];

    // Index of window that displays currently selected buffer.
    static int current_window_idx;

    // Index of active window before entering minibuffer.
    // TODO(Cleaup): Default value.
    static int window_idx_before_entering_minibuffer;
}

enum WindowTraverseMode { WIN_TRAVERSE_FORWARD, WIN_TRAVERSE_BACKWARDS };

static EditorBuffer *CreateNewBuffer()
{
    globals::buffers[globals::number_of_buffers++] = { .color = WINDOW_COLOR };
    return globals::buffers + globals::number_of_buffers - 1;
}

static EditorWindow *CreateNewWindowWithBuffer(EditorBuffer *buffer,
                                               EditorWindow *root_window)
{
    globals::windows_arr[globals::number_of_windows++] =
        EditorWindow
        {
            .contains_buffer = 1,
            .buffer_ptr = buffer,
            .parent_ptr = root_window
        };

    return globals::windows_arr + globals::number_of_windows - 1;
}

// NOTE: This window must contain buffer!
// NOTE: This heavily assumes that this window is the active one.
// This focus is set to the buffer that is in the window before the split.
void EditorWindow::SplitWindow(WindowSplit split_type)
{
    if (contains_buffer)
    {
        EditorBuffer *new_buffer = CreateNewBuffer();
        EditorBuffer *previous_buffer_ptr = buffer_ptr;

        // parent_ptr cannot contain buffer!
        assert(!parent_ptr || !parent_ptr->contains_buffer);

        if (!parent_ptr || parent_ptr->split != split_type)
        {
            // We must create new window, which holds these two:
            split = split_type;
            splits_percentages[0] = 0.5f;
            split_windows[0] = CreateNewWindowWithBuffer(previous_buffer_ptr, this);
            split_windows[1] = CreateNewWindowWithBuffer(new_buffer, this);
            number_of_windows = 2;
            globals::current_window_idx = split_windows[0] - globals::windows_arr;
            contains_buffer = 0;
            UpdateSize(position);
        }
        else
        {
            assert(parent_ptr->number_of_windows < MAX_WINDOWS_ON_SPLIT - 1);

            // TODO(Next):
            //      -> Get index in parent window,
            //      -> Move windows with bigger index by one in the array.

            //      -> Calculate the split which is half between split percent
            //         under index of THIS buffer, and the next one nad insert
            //         new window with buffer 'new_buffer' there.
        }
    }
    else
    {
        // TODO(Debug): Info, that something has gone horribly wrong...
        assert(false);
    }
}

void EditorWindow::UpdateSize(Rect new_rect)
{
    position = new_rect;

    // If windows contains multiple windows, we must update their size first.
    if (!contains_buffer)
    {
        // Tells at which pixel one of the windows in the split ends.
        // In range of (min_pos - max_pos);
        int split_idx[MAX_WINDOWS_ON_SPLIT];

        assert(number_of_windows >= 2);

        for (int i = 0; i < number_of_windows-1; ++i)
            assert(splits_percentages[i] > 0 &&
                   splits_percentages[i] < 1 &&
                   (i < number_of_windows-2
                    ? splits_percentages[i] < splits_percentages[i+1]
                    : 1));

        int min_pos = (split == WindowSplit::WIN_SPLIT_HORIZONTAL
                       ? position.x : position.y);
        int max_pos = (split == WindowSplit::WIN_SPLIT_HORIZONTAL
                       ? position.x + position.width : position.y + position.height);
        int difference = max_pos - min_pos;
        assert(difference > 0);

        for (int i = 0; i < number_of_windows-1; ++i)
        {
            split_idx[i] =
                min_pos + static_cast<int>(splits_percentages[i] * difference);

            // TODO(Splitting lines): Mayby just handle them as part of the buffer?
            // There is a one pixel space for splitting line.
            if (i > 0)
                split_idx[i] += 1;

            assert(min_pos < split_idx[i] && split_idx[i] < max_pos);
        }
        split_idx[number_of_windows-1] = max_pos;

        int previous_split = min_pos;
        for (int i = 0; i < number_of_windows; ++i)
        {
            Rect nested_window_rect =
                (split == WindowSplit::WIN_SPLIT_HORIZONTAL
                 ? Rect { previous_split, position.y,
                          split_idx[i]-previous_split, position.height }
                 : Rect { position.x, previous_split,
                          position.width, split_idx[i]-previous_split });

#if 0
            // TODO(Profiling): Move outside the loop for better performace?
            if (i < number_of_windows - 1)
            {
                Rect splitting_rect = (split == WindowSplit::WIN_SPLIT_HORIZONTAL
                                       ? Rect { split_idx[i], rect.y, 1, rect.height }
                                       : Rect { rect.x, split_idx[i], rect.width, 1 });

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
        const SDL_Rect sdl_rect = {
            position.x, position.y,
            position.width, position.height
        };

        SDL_FillRect(globals::screen, &sdl_rect,
                     current_select ? 0xddee22 : buffer_ptr->color);
    }
    else
        for (int i = 0; i < number_of_windows; ++i)
        {
            split_windows[i]->Redraw(globals::current_window_idx ==
                                     (split_windows[i] - globals::windows_arr));
        }
}

// TODO(Cleanup): Would be nice to add them using regular, add-window/buffer API.
static void InitializeFirstWindow()
{
    globals::number_of_buffers = 2;
    globals::buffers[0] = { .color = 0xffffff }; // Buffer with index 0 is a minibufer.
    globals::buffers[1] = { .color = WINDOW_COLOR };

    globals::number_of_windows = 2;

    // Window with index 0 contains minibuffer. And is drawn separetely.
    globals::windows_arr[0] = EditorWindow {
        .contains_buffer = 1,
        .buffer_ptr = globals::buffers,
        .parent_ptr = nullptr,
        .position = Rect { 0, globals::window_h - 17 +1, globals::window_w, 17 }
    };

    // Window with index 1 is main window.
    globals::windows_arr[1] = EditorWindow {
        .contains_buffer = 1,
        .buffer_ptr = globals::buffers + 1,
        .parent_ptr = nullptr,
        .position = Rect { 0, 0, globals::window_w, globals::window_h - 17 }
    };

    globals::current_window_idx = 1;
}

// TODO(Splitting lines): Remove unused atrribute!
__attribute__ ((unused))
static void DrawSplittingLine(const Rect &rect)
{
    SDL_Rect split_line = SDL_Rect { rect.x, rect.y, rect.width, rect.height };
    SDL_FillRect(globals::screen, &split_line, 0x64645e);
}

// TODO(Cleaup): Create namespace application and move resize and redraw window
// functions and all globals there!
// TODO(Cleanup): Check if something here can fail, if not change the type to void.
static int ResizeWindow()
{
    globals::windows_arr[0].UpdateSize(Rect {
            0, globals::window_h - 17 +1,
            globals::window_w, 17 });
    globals::windows_arr[1].UpdateSize(Rect {
            0, 0,
            globals::window_w, globals::window_h - 17 });

    return 0;
}

// TODO(Cleaup): Check if something here can fail, if not change the type to void.
static int RedrawWindow()
{
    globals::screen = SDL_GetWindowSurface(globals::window);
    if (!globals::screen)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "\e[91;1mCouldnt get the right surface of the window!\e[0m");
        assert(false);
    }

    SDL_GetWindowSize(globals::window, &globals::window_w, &globals::window_h);

    if (globals::window_w == 0 || globals::window_h == 0)
        assert(!"Size is 0");

    // Fill the whole screen, just in case.
    const SDL_Rect whole_screen = { 0, 0, globals::window_w, globals::window_h };
    SDL_FillRect(globals::screen, &whole_screen, 0xff00ff); // Color suggesting error.

    assert(globals::number_of_windows > 0);
    const EditorWindow main_window = globals::windows_arr[1];
    const EditorWindow mini_buffer_window = globals::windows_arr[0];

    // TODO(Splitting lines): Decide how i want to draw them.
#if 0
    DrawSplittingLine({ 0, globals::window_h - 17, globals::window_w, 1 });
#endif

    mini_buffer_window.Redraw(globals::current_window_idx == 0);
    main_window.Redraw(globals::current_window_idx == 1);

    return 0;
}

// If traverse is forward, we pick the first window in the subtree of the
// root_window, otherwise we pick the last one (we always go either left or
// right recusively).
static EditorWindow *GetFirstOrLastWindowInSubtree(EditorWindow *const root_window,
                                                   const WindowTraverseMode traverse)
{
    // If root window is a bufer, it is just selected.
    if (root_window->contains_buffer)
        return root_window;

    EditorWindow *next_window = root_window;
    int window_to_pick_idx;

    if (traverse == WindowTraverseMode::WIN_TRAVERSE_FORWARD)
        window_to_pick_idx = 0;
    else
    {
        assert (traverse == WindowTraverseMode::WIN_TRAVERSE_BACKWARDS);
        window_to_pick_idx = next_window->number_of_windows - 1;
    }

    while (!next_window->contains_buffer)
        next_window = next_window->split_windows[window_to_pick_idx];

    return next_window;
}

static EditorWindow *GetNextOrPrevActiveWindow(const EditorWindow *current_window,
                                               const WindowTraverseMode traverse)
{
    EditorWindow *parent = current_window->parent_ptr;
    if (!parent)
        return GetFirstOrLastWindowInSubtree(globals::windows_arr + 1, traverse);

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

    // TODO(Cleaup): Compress this better!
    if (traverse == WindowTraverseMode::WIN_TRAVERSE_FORWARD)
    {
        if (index_in_parent + 1 >= parent->number_of_windows)
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
                parent->split_windows[index_in_parent + 1],
                traverse);
        }
    }

    else
    {
        assert(traverse == WindowTraverseMode::WIN_TRAVERSE_BACKWARDS);
        if (index_in_parent == 0)
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
                parent->split_windows[index_in_parent - 1],
                traverse);
        }
    }
}

// If go_forward is true we switch to the next widnow, otherwise we switch to
// the previous one.
static void SwitchWindow(const WindowTraverseMode traverse)
{
    if (globals::current_window_idx == 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "\e[91;1mCannot switch window in minibuffer!\e[0m");
        return;
    }

    // TODO(Cleaup): Do we assume there is at least one buffer at a time?
    // TODO(Cleaup): What if there is just minibuffer?
    assert(globals::number_of_buffers);
    EditorWindow *next_window = nullptr;

    next_window = GetNextOrPrevActiveWindow(
        globals::windows_arr + globals::current_window_idx,
        traverse);

    assert(next_window);
    globals::current_window_idx = next_window - globals::windows_arr;
}

static void SwitchToMiniBuffer()
{
    if (globals::current_window_idx == 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "\e[91;1mAlready in minibuffer\e[0m");
        return;
    }

    globals::window_idx_before_entering_minibuffer
        = globals::current_window_idx;
    globals::current_window_idx = 0;
}

static void SwitchOutFromMiniBuffer()
{
    if (globals::current_window_idx != 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "\e[91;1mNot in minibuffer\e[0m");
        return;
    }

    globals::current_window_idx
        = globals::window_idx_before_entering_minibuffer;
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
                if (globals::current_window_idx != 0)
                {
                    globals::windows_arr[globals::current_window_idx].
                        SplitWindow(WindowSplit::WIN_SPLIT_HORIZONTAL);
                }
                else
                {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                                 "\e[93;1mCannot split minibuffer!\e[0m");
                }
            }
            // V - Split vertically.
            else if (event.key.keysym.sym == 118)
            {
                if (globals::current_window_idx != 0)
                {
                    globals::windows_arr[globals::current_window_idx].
                        SplitWindow(WindowSplit::WIN_SPLIT_VERTCAL);
                }
                else
                {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                                 "\e[93;1mCannot split minibuffer!\e[0m");
                }
            }

            if (globals::windows_arr[globals::current_window_idx].contains_buffer)
            {
                SDL_Log("Current window: %d (with color: %x)",
                        globals::current_window_idx,
                        globals::windows_arr[globals::current_window_idx]
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
                    ResizeWindow();
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
    globals::window = SDL_CreateWindow("Editor",
                                     SDL_WINDOWPOS_UNDEFINED,
                                     SDL_WINDOWPOS_UNDEFINED,
                                     width, height, 0);
                                     // 0, 0, 0); // To test corner-cases.

    if (!globals::window)
    {
        // TODO(Debug): Logging.
        return -1;
    }

    // Instead of creating a renderer, draw directly to the screen.
    globals::screen = SDL_GetWindowSurface(globals::window);

    if (!globals::screen)
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
        WindowSplit split = window->split;
        for (int i = 0; i < window->number_of_windows; ++i)
            if (window->split_windows[i]->split == split)
                return false;

        bool invalid = false;
        for (int i = 0; i < window->number_of_windows; ++i)
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
        SDL_Log("Could not get display mode for video display #%d: %s",
                0, SDL_GetError());
    else
        // On success, print the current display mode.
        SDL_Log("Display #%d: current display mode is %dx%dpx @ %dhz.",
                0, current.w, current.h, current.refresh_rate);

    // The size of the window to render.
    globals::window_w = current.w / 3;
    globals::window_h = current.h / 2;

    int window_x = current.w / 2 - globals::window_w / 2,
        window_y = current.h / 2 - globals::window_h / 2;

    // TODO(Platform): Use values based on screen width/height, or let user set his own.
    if (FAILED(InitWindow(globals::window_w, globals::window_h)))
    {
        // TODO(Debug): Logging.
        exit(1);
    }
    SDL_SetWindowPosition(globals::window, window_x, window_y);

    InitializeFirstWindow();

    // TODO(Platform): Make it same as background color!
    const SDL_Rect foo = { 0, 0, globals::window_w, globals::window_h };
    SDL_FillRect(globals::screen, &foo, 0x272822);

#if 0
    assert(globals::bitmap_surface);
#endif

    TTF_Font *font_test =
        TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSansMono.ttf", globals::font_size);

    if (!(globals::text_surface[0] = TTF_RenderText_Solid(font_test,
                                                 "This text is: Solid",
                                                 (SDL_Color){0xF8, 0xF8, 0xF8, 255}))

        || !(globals::text_surface[1] = TTF_RenderText_Shaded(font_test,
                                                     "This text is: Shaded",
                                                     (SDL_Color){0xF8, 0xF8, 0xF8, 255},
                                                     (SDL_Color){0x15, 0x15, 0x80, 0xFF}))

        || !(globals::text_surface[2] = TTF_RenderText_Blended(font_test,
                                                      "Compilation started at Fri Jun",
                                                      (SDL_Color){0xF8, 0xF8, 0xF8, 255}))

        || !(globals::text_surface[3] = TTF_RenderText_Blended(font_test,
                                                      "#define",
                                                      (SDL_Color){0xF9, 0x26, 0x72, 255})))
    {
        // TODO(Debug): handle error here, perhaps print TTF_GetError at least.
        assert(false);
    }

    TTF_CloseFont(font_test);
    font_test = nullptr;

    for (;;)
    {
        int width, height;
        SDL_GetWindowSize(globals::window, &width, &height);
        SDL_Log("SIZE: %d x %d\n", width, height);

        SDL_Event event;
        SDL_WaitEvent(&event);

        if (HandleEvent(event))
            break;

        static float diff = 0.01f;
        globals::windows_arr[0].splits_percentages[0] += diff;
        if (globals::windows_arr[0].splits_percentages[0] > 0.9f)
        {
            globals::windows_arr[0].splits_percentages[0] = 0.9f;
            diff *= -1;
        }
        else if (globals::windows_arr[0].splits_percentages[0] < 0.1f)
        {
            globals::windows_arr[0].splits_percentages[0] = 0.1f;
            diff *= -1;
        }

        // Move it to some event occurences or make a dirty-bit system.
        // TODO(Profiling): Redraw window only if something got dirty.
        RedrawWindow();

        if (!Validate(globals::windows_arr + 1))
            assert(false);
    }

    TTF_Quit();
    SDL_Quit();
    return 0;
}
