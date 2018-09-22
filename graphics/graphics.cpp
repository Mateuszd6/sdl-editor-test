// TODO(Splitting lines): Remove unused atrribute!
#include "graphics.hpp"
#include "../platfrom/platform.hpp"

#include <SDL.h>

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

    static void print_text_line(editor::window const* window_ptr,
                                char const* text,
                                bool color,
                                int line_nr, // First visible line of the buffer is 0.
                                int cursor_idx, // gap_buffer const* line
                                int first_line_offset,
                                bool start_from_top)
    {
        auto horizontal_offset = 2;
        auto vertical_offest = 2 + first_line_offset;

        for (auto i = 0; text[i]; ++i)
        {
            auto text_idx = static_cast<int>(text[i]);
            auto draw_rect = rectangle {
                // Only for monospace fonts.
                window_ptr->position.x + horizontal_offset + ::platform::get_letter_width() * i,
                window_ptr->position.y + vertical_offest + ::platform::get_letter_height() * line_nr,
                ::platform::get_letter_width(),
                ::platform::get_letter_height()
            };

#if 0
            if (draw_rect.x + draw_rect.width > window_ptr->position.x + window_ptr->position.width
                || draw_rect.y + draw_rect.height > window_ptr->position.y + window_ptr->position.height)
            {
                LOG_WARN("Blitting outside the rect: %s", text);
                break;
            }
#endif
            if(color)
                ::platform::blit_letter_colored(text_idx, draw_rect);
            else
                ::platform::blit_letter(text_idx, draw_rect);
        }

        // TODO: Make sure that the cursor fits in the window.
        // Draw a cursor.
        if (cursor_idx >= 0)
        {
            auto rect = rectangle {
                window_ptr->position.x + horizontal_offset + ::platform::get_letter_width() * cursor_idx,
                window_ptr->position.y + vertical_offest + ::platform::get_letter_height() * line_nr,
                2,
                ::platform::get_letter_height()
            };

            ::platform::draw_rectangle_on_screen(rect, make_color(0xF8F8F8FF));
        }
    }
}
