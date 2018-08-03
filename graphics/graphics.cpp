// TODO(Splitting lines): Remove unused atrribute!
#include "graphics.hpp"
#include "../platfrom/platform.hpp"

namespace graphics
{
    static color make_color(uint32 value)
    {
        return color { .int_color = value, };
    }

    static void DrawSplittingLine(rectangle const& rect)
    {
        // auto split_line = SDL_Rect { rect.x, rect.y, rect.width, rect.height };
        platform::draw_rectangle_on_screen(rect, make_color(0xFF00FF));
    }
}
