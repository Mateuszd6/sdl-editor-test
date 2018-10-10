#include "window.hpp"
#include "../platfrom/platform.hpp"
#include "../graphics/graphics.hpp"

// TODO(Cleanup): Try to use as less globals as possible...
namespace editor::global
{
    // Window at index 0 is main window created on init and it cant be removed.
    // TODO(Testing): Make these auto-resizable and think about initial size.
    static int number_of_windows = 0;
    static ::editor::window windows_arr[256];

    // Index of window that displays currently selected buffer.
    static int64 current_window_idx;

    // Index of active window before entering minibuffer.
    // TODO(Cleanup): Default value.
    static int64 window_idx_before_entering_minibuffer;
}

namespace editor
{
    static window *CreateNewWindowWithBuffer(buffer* buffer,
                                             window* root_window)
    {
        // TODO: Clear this shit and make a proper ctor.
        global::windows_arr[global::number_of_windows++] =
            window
            {
                .contains_buffer = 1,
                .buf_point = create_buffer_point(buffer),
                .parent_ptr = root_window,
            };

        return global::windows_arr + global::number_of_windows - 1;
    }

#include <unistd.h>
    // TODO(Cleanup): Would be nice to add them using regular, add-window/buffer API.
    static void initialize_first_window()
    {
        auto mini_buffer = CreateNewBuffer();

        char buf[256];

        // Linux specyfic way to get the application root path.
        // TODO: Move to platform namespace etc.
        {

            auto rl_result = readlink("/proc/self/exe", buf, 220); // TODO: handle longer names.
            auto last = strrchr(buf, '/');
            ASSERT(last != nullptr);
            last++;
            auto filename = "main.cpp";
            auto i = 0;
            for(auto fn = filename; *fn;)
                *last++ = *fn++;
            *last = 0_i8;

            if(rl_result == -1)
                PANIC("Could not get the application absolute path!");
        }

        auto main_window_buffer = create_buffer_from_file(buf);

        global::number_of_windows = 2;
        // Window with index 0 contains minibuffer. And is drawn separetely.
        global::windows_arr[0] = window {
            .contains_buffer = 1,
            .buf_point = create_buffer_point(mini_buffer),
            .parent_ptr = nullptr,
            .position = ::graphics::rectangle {
                0, ::graphics::global::window_h - 17 + 1,
                ::graphics::global::window_w, 17
            }
        };

        // Window with index 1 is main window.
        global::windows_arr[1] = window {
            .contains_buffer = 1,
            .buf_point = create_buffer_point(main_window_buffer),
            .parent_ptr = nullptr,
            .position = ::graphics::rectangle {
                0, 0,
                ::graphics::global::window_w, ::graphics::global::window_h - 17
            }
        };

        global::current_window_idx = 1;
    }

#if 0
    // TODO: Remove unused atrribute!
    __attribute__ ((unused))
    static void DEBUG_PrintWindowsState(const window *window)
    {
        if (!window)
            printf(" (nullptr) ");
        else if (window->contains_buffer)
            printf(" (Buffer: %ld) ", (window->buf_point.buffer_ptr - global::buffers);
        else
        {
            printf(" (%c){ ",
                   window->split == window_split::WIN_SPLIT_HORIZONTAL ? 'H' : 'V');
            for(auto i = 0; i < window->number_of_windows; ++i)
                DEBUG_PrintWindowsState(window->split_windows[i]);
            printf("} ");
        }
    }
#endif

    int window::GetIndexInParent() const
    {
        auto result = -1;
        for (auto i = 0; i < parent_ptr->number_of_windows; ++i)
            if (parent_ptr->split_windows[i] == this)
            {
                result = i;
                break;
            }
        ASSERT(result >= 0);
        return result;
    }

    int window::GetDimForSplitType(window_split split_type) const
    {
        switch (split_type)
        {
            case WIN_SPLIT_VERTCAL:
                return position.height;
            case WIN_SPLIT_HORIZONTAL:
                return position.width;
        }
    }

    float window::GetScreenPercForSplitType(window_split split_type) const
    {
        switch (split_type)
        {
            case WIN_SPLIT_VERTCAL:
                return static_cast<float>(position.height) /  ::graphics::global::window_h;
            case WIN_SPLIT_HORIZONTAL:
                return static_cast<float>(position.width) / ::graphics::global::window_w;
        }
    }

    bool window::IsFirstInParent() const
    {
        return (parent_ptr->split_windows[0] == this);
    }

    bool window::IsLastInParent() const
    {
        return (parent_ptr->split_windows[parent_ptr->number_of_windows - 1] == this);
    }

    // NOTE: This window must contain buffer!
    // NOTE: This heavily assumes that this window is the active one.
    // This focus is set to the buffer that is in the window before the split.
    void window::SplitWindow(window_split split_type)
    {
        // We won't let user split the window, if one of the windows takes less than
        // the min size of the window.
        auto split_h_or_v = (split_type == WIN_SPLIT_VERTCAL
                             ? static_cast<float>(position.height) / ::graphics::global::window_h
                             : static_cast<float>(position.width) / ::graphics::global::window_w);

        if (split_h_or_v < MIN_PERCANTAGE_WINDOW_SPLIT * 2)
        {
            LOG_ERROR("Cannot split. The window is too small");
            return;
        }

        if (contains_buffer)
        {
            // TODO: This is obsolete!
            auto new_buffer = CreateNewBuffer();
            auto previous_buffer_ptr = CreateNewBuffer(); // buf_point.buffer_ptr;

            // parent_ptr cannot contain buffer!
            ASSERT(!parent_ptr || !parent_ptr->contains_buffer);

            if (!parent_ptr || parent_ptr->split != split_type)
            {
                // We must create new window, which holds these two:
                split = split_type;
                splits_percentages[0] = 0.5f;
                splits_percentages[1] = 1.0f;
                split_windows[0] = CreateNewWindowWithBuffer(previous_buffer_ptr, this);
                split_windows[1] = CreateNewWindowWithBuffer(new_buffer, this);
                number_of_windows = 2;
                global::current_window_idx = split_windows[0] - global::windows_arr;
                contains_buffer = 0;
                UpdateSize(position);
            }
            else
            {
                // TODO(Cleanup): Make sure that this assertion won't fire.
                ASSERT(parent_ptr->number_of_windows < MAX_WINDOWS_ON_SPLIT - 1);

                auto idx_in_parent = GetIndexInParent();

                // Move windows with bigger index by one in the array.
                for (auto i = parent_ptr->number_of_windows;
                     i >= idx_in_parent + 1;
                     --i)
                {
                    parent_ptr->split_windows[i] = parent_ptr->split_windows[i-1];
                    parent_ptr->splits_percentages[i] = parent_ptr->splits_percentages[i-1];
                }
                parent_ptr->split_windows[idx_in_parent + 1] =
                    CreateNewWindowWithBuffer(new_buffer, parent_ptr);

                // Calculate the split which is half between split percent under
                // index of THIS buffer, and the next one nad insert new window with
                // buffer 'new_buffer' there.
                auto previous_split = (idx_in_parent == 0
                                       ? 0
                                       : parent_ptr->splits_percentages[idx_in_parent - 1]);
                auto split_perct =
                    (parent_ptr->splits_percentages[idx_in_parent] + previous_split) / 2;
                parent_ptr->splits_percentages[idx_in_parent] = split_perct;

                // We can safetly increment this, becasue of the check above.
                parent_ptr->number_of_windows++;

                // TODO(Cleanup): i think this should be false, because parent must
                // be split window (does not contain a single buffer), and therefore
                // cannot be selected.
                parent_ptr->UpdateSize(parent_ptr->position);
                parent_ptr->redraw(false);
            }
        }
        else
            PANIC("This windows cannot be splited and should never ever be focused!");
    }

    // TODO(Cleanup): Summary
    void window::UpdateSize(graphics::rectangle new_rect)
    {
        position = new_rect;

        // If windows contains multiple windows, we must update their size first.
        if (!contains_buffer)
        {
            // Tells at which pixel one of the windows in the split ends.
            // In range of (min_pos - max_pos);
            int split_idx[MAX_WINDOWS_ON_SPLIT];

            ASSERT(number_of_windows >= 2);

            for (auto i = 0; i < number_of_windows-1; ++i)
                ASSERT(splits_percentages[i] > 0 &&
                       splits_percentages[i] < 1 &&
                       (i < number_of_windows-2
                        ? splits_percentages[i] < splits_percentages[i+1]
                        : 1));

            auto min_pos = (split == window_split::WIN_SPLIT_HORIZONTAL
                            ? position.x : position.y);
            auto max_pos = (split == window_split::WIN_SPLIT_HORIZONTAL
                            ? position.x + position.width : position.y + position.height);
            auto difference = max_pos - min_pos;
            ASSERT(difference > 0);

            for (auto i = 0; i < number_of_windows-1; ++i)
            {
                split_idx[i] = min_pos + static_cast<int>(splits_percentages[i] * difference);

                // TODO(Splitting lines): Mayby just handle them as part of the buffer?
                // There is a one pixel space for splitting line.
                if (i > 0)
                    split_idx[i] += 1;

                ASSERT(min_pos < split_idx[i] && split_idx[i] < max_pos);
            }
            split_idx[number_of_windows-1] = max_pos;

            auto previous_split = min_pos;
            for (auto i = 0; i < number_of_windows; ++i)
            {
                auto nested_window_rect =
                    (split == window_split::WIN_SPLIT_HORIZONTAL
                     ? graphics::rectangle { previous_split, position.y,
                            split_idx[i]-previous_split, position.height }
                     : graphics::rectangle { position.x, previous_split,
                               position.width, split_idx[i]-previous_split });

#if 1
                // TODO(Profiling): Move outside the loop for better performace?
                if (i < number_of_windows - 1)
                {
                    auto splitting_rect =
                        (split == window_split::WIN_SPLIT_HORIZONTAL
                         ? graphics::rectangle { split_idx[i], nested_window_rect.y,
                                1, nested_window_rect.height }
                         : graphics::rectangle { nested_window_rect.x, split_idx[i],
                                nested_window_rect.width, 1 });

                    // TODO(Cleanup): This should not be here, as it is not
                    // associated with this fuction. Check the inclueds after
                    // doing this!
                    graphics::DrawSplittingLine(splitting_rect);
                }
#endif // 0

                // Add 1 becasue this is a space for the line separator.
                previous_split = split_idx[i]+1;
                split_windows[i]->UpdateSize(nested_window_rect);
            }
        }
    }

    void window::redraw(bool current_select) const
    {
        if (contains_buffer)
        {
            ::platform::draw_rectangle_on_screen(position, ::graphics::make_color(0xFFFFFF)); // 0x272822
        }
        else
            for (auto i = 0; i < number_of_windows; ++i)
            {
                split_windows[i]->redraw(global::current_window_idx ==
                                         (split_windows[i] - global::windows_arr));
            }
    }
}
