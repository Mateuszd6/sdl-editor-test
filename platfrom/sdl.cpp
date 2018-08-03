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
    static SDL_Surface *screen;

    static const int number_of_surfaces = 5;
    static SDL_Surface *alphabet['z' - 'a' + 1];
    static SDL_Surface *text_surface[number_of_surfaces];
}

namespace platform
{
    static void draw_rectangle_on_screen(graphics::rectangle const& rect,
                                         graphics::color const& col)
    {
        auto sdl_rect = SDL_Rect { rect.x, rect.y, rect.width, rect.height };
        SDL_FillRect(global::screen, &sdl_rect, col.int_color);
    }
}
