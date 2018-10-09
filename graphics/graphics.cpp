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

    static void print_text_line(editor::window const* window_ptr,
                                char const* text,
                                bool color,
                                int64 line_nr, // First visible line of the buffer is 0.
                                int64 cursor_idx,
                                int32 first_line_offset,
                                bool start_from_top)
    {
        auto horizontal_offset = 2;
        auto vertical_offest = 2 + first_line_offset;

        auto X = static_cast<int32>(window_ptr->position.x + horizontal_offset);
        auto Y = static_cast<int32>(window_ptr->position.y + vertical_offest + ::platform::get_line_height() * (line_nr + 1));
        for (auto i = 0; text[i]; ++i)
        {
            auto text_idx = static_cast<int16>(text[i]);
            auto draw_rect = rectangle {
                // Only for monospace fonts.
                X,
                Y,
                ::platform::get_letter_width(),
                ::platform::get_letter_height()
            };

            // TODO: This does not make much sense. Take a look at it!
            auto fixed_height = window_ptr->position.y + window_ptr->position.height - draw_rect.y;

            // TODO: This looks like a reasonable default:
            auto advance = ::platform::get_letter_width();
            if(color)
                ::platform::blit_letter_colored(text_idx, fixed_height, X, Y, &advance);
            else
                ::platform::blit_letter(text_idx, fixed_height, X, Y, &advance);

            X += advance;

        }

        // TODO: Make sure that the cursor fits in the window.
        // Draw a cursor.
        if (cursor_idx >= 0)
        {
            auto rect = rectangle {
                static_cast<int32>(window_ptr->position.x + horizontal_offset + ::platform::get_letter_width() * cursor_idx),
                static_cast<int32>(window_ptr->position.y + vertical_offest  + ::platform::get_line_height() * line_nr),
                2,
                ::platform::get_line_height() + vertical_offest
            };

            ::platform::draw_rectangle_on_screen(rect, make_color(0x0));
        }
    }
}
