// TODO(Splitting lines): Remove unused atrribute!
#include "graphics.hpp"

namespace graphics
{
    __attribute__ ((unused))
    static void DrawSplittingLine(graphics::rectangle const& rect)
    {
        auto split_line = SDL_Rect { rect.x, rect.y, rect.width, rect.height };
        SDL_FillRect(global::screen, &split_line, 0x000000); // 0x64645e
    }
}
