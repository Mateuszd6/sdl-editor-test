// TODO(Cleaup): This should be totally refactored!
#ifndef __EDITOR_WINDOW_HPP_INCLUDED__
#define __EDITOR_WINDOW_HPP_INCLUDED__

#include "../graphics/graphics.hpp"

#define MAX_WINDOWS_ON_SPLIT (10)
#define MIN_PERCANTAGE_WINDOW_SPLIT (0.2f)

namespace editor
{
    // To lowercae!
    enum window_split { WIN_SPLIT_VERTCAL, WIN_SPLIT_HORIZONTAL };
    enum window_traverse_mode { WIN_TRAVERSE_FORWARD, WIN_TRAVERSE_BACKWARDS };

    struct window
    {
        // If 0, window is splited to multiple windows, either vertically or
        // horizontally. Otherwise pointer to buffer, that is displayed in
        // window is stored.
        int contains_buffer;

        // A pointer to the splitting window, which owns this one, or nullptr,
        // if this is a top window. (If nullptr, there should be only one window
        // in the screen (expect minibuffer)).
        window *parent_ptr;

        // This must be updated after every window resize.
        graphics::rectangle position;

        union
        {
            struct // If contains single buffer:
            {
                editor::buffer_point buf_point;
            };
            struct // If is splited into multiple windows:
            {
                window_split split;

                // Must be < MAX_WINDOWS_ON_SPLI and >= 2
                int number_of_windows;

                // Splits percentages must be increasing and in range 0-1.
                float splits_percentages[MAX_WINDOWS_ON_SPLIT - 1];
                window *split_windows[MAX_WINDOWS_ON_SPLIT];
            };
        };

        void SplitWindow(window_split split_type);
        void UpdateSize(graphics::rectangle new_rect);
        void redraw(bool current_select) const;
        int GetIndexInParent() const;
        bool IsFirstInParent() const;
        bool IsLastInParent() const;

        int GetDimForSplitType(window_split split_type) const;
        float GetScreenPercForSplitType(window_split split_type) const;
    };
}

#endif // __EDITOR_WINDOW_HPP_INCLUDED__
