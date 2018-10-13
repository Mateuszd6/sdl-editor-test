// TODO(Splitting lines): Remove unused atrribute!
#include "graphics.hpp"
#include "../platfrom/platform.hpp"

// TODO: Get rid of SDL here!
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <SDL.h>
#pragma clang diagnostic pop


namespace graphics
{
    static color make_color(uint32 value)
    {
        return color { .int_color = value, };
    }

    // TODO: Snake case!
    static void DrawSplittingLine(rectangle const& rect)
    {
        // auto split_line = SDL_Rect { rect.x, rect.y, rect.width, rect.height };
        platform::draw_rectangle_on_screen(rect, make_color(0xFF00FF));
    }
}
