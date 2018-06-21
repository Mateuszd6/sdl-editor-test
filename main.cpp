#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <SDL.h>
#include <SDL_ttf.h>

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
static int font_size = 15;
static int vertical_padding = 5;
static int horizontal_padding = 15;
static int first_item_height = 5;

// TODO: Temporary
static float global_screen_split_percentage = 0.8f;
static float global_screen_split_percentage_delta = 0.01;

union EditorWindow
{
    // TODO(Tomorrow): START HERE.
    int contains_buffer;
    struct
    {

    };
    struct
    {

    };
};

// TODO: Check if any of these functions can fail and handle this.
static int ResizeAndRedrawWindow()
{
    SDL_UpdateWindowSurface(global_window);
    global_screen = SDL_GetWindowSurface(global_window);
    SDL_GetWindowSize(global_window, &global_window_w, &global_window_h);

    const SDL_Rect whole_screen = { 0, 0, global_window_w, global_window_h };
    SDL_FillRect(global_screen, &whole_screen, 0x171812); // Monokai theme: 0x272822

    assert(global_screen_split_percentage);
    int line_split_px = static_cast<int>(global_screen_split_percentage * global_window_w);

    // TODO: This needs more abstraction when there are more splits, recusive
    // splits etc, etc...
    SDL_Rect buffers[2];
    buffers[0] = { 0, 0, line_split_px, global_window_h };
    buffers[1] = { line_split_px, 0, global_window_w - line_split_px, global_window_h };

    for (int i = 0; i < 2; ++i)
    {
        int color = (i == 0 ? 0x991111 : 0x119911);
        SDL_FillRect(global_screen, buffers+i, color);
    }

    const SDL_Rect splitting_line = { line_split_px, 0, 1, global_window_h };
    SDL_FillRect(global_screen, &splitting_line, 0xFFFFFF);

    SDL_Rect rects[number_of_surfaces];
    for (int i = 0; i < number_of_surfaces; ++i)
    {
        rects[i] = {
            horizontal_padding,
            first_item_height + i * (font_size + vertical_padding),
            text_surface[i]->w,
            text_surface[i]->h
        };
    }

    for (int i = 0; i < number_of_surfaces; ++i)
        SDL_BlitSurface(text_surface[i], nullptr, global_screen, rects + i);

    return 0;
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
            SDL_Log("KEY DOWN %d\n", event.key.keysym.sym);
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
    global_window = SDL_CreateWindow("Editor",
                                     SDL_WINDOWPOS_UNDEFINED,
                                     SDL_WINDOWPOS_UNDEFINED,
                                     width, height, 0);
    // NOTE(Testing): Sometimes it is good to set width and height to 0, 0 and
    // test what happens when window is smallest possible.

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

    // TODO: Make it same as background color!
    const SDL_Rect foo = { 0, 0, global_window_w, global_window_h };
    SDL_FillRect(global_screen, &foo, 0x272822);

#if 0
    assert(global_bitmap_surface);
#endif

    TTF_Font *font_test =
        TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSansMono.ttf", font_size);

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

        if (global_screen_split_percentage > 0.9f)
        {
            global_screen_split_percentage = 0.89f;
            global_screen_split_percentage_delta *= -1;
        }
        else if (global_screen_split_percentage < 0.1f)
        {
            global_screen_split_percentage = 0.11f;
            global_screen_split_percentage_delta *= -1;
        }

        global_screen_split_percentage += global_screen_split_percentage_delta;

        // Move it to some event occurences or make a dirty-bit system.
        ResizeAndRedrawWindow();
    }

    TTF_Quit();
    SDL_Quit();
    return 0;
}
