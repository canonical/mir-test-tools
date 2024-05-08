#include "decorated_xdg_toplevel_window.h"
#include "globals.h"
#include "mir_window.h"

#include <cairo.h>
#include <linux/input-event-codes.h>

#include <functional>
#include <numbers>
#include <string>

namespace mfa = mir_flutter_app;

mfa::DecoratedXdgToplevelWindow::DecoratedXdgToplevelWindow(
    wl_surface* surface,
    int32_t width,
    int32_t height,
    Configuration config) :
    XdgToplevelWindow(surface, width, height),
    config_{std::move(config)}
{
}

void mfa::DecoratedXdgToplevelWindow::handle_mouse_button(
    wl_pointer* pointer,
    uint32_t serial,
    uint32_t time,
    uint32_t button,
    uint32_t state)
{
    XdgToplevelWindow::handle_mouse_button(pointer, serial, time, button, state);

    // Prevent the window from being closed if it has a dialog descendant.
    //
    // From the protocol:
    //     (1) A user should be able to move, resize, or hide an open dialog’s parent, but not
    //     close it or interact with its contents; and move, resize, or close the parent’s
    //     satellites if any, but not interact with their contents.
    //
    // TODO: Should this behavior be handled by the compositor?
    auto const any_descendant{[](MirWindow* window, std::function<bool(MirWindow*)> const& pred) -> bool
        {
            auto const impl{[&pred](MirWindow* window, auto const& self)
                {
                    if (pred(window))
                        return true;

                    return std::any_of(
                        window->children.begin(),
                        window->children.end(),
                        [&self](MirWindow* child) { return self(child, self); });
                }};

            return std::any_of(
                window->children.begin(),
                window->children.end(),
                [&](MirWindow* window) { return impl(window, impl); });
        }};
    if (any_descendant(
        Globals::instance().window_for(static_cast<wl_surface*>(*this)),
        [](MirWindow* descendant) { return descendant->archetype == MirWindowArchetype::dialog; }))
    {
        return;
    }

    auto const pointer_on_close_button{[this]
        {
            auto const margin{10};
            auto const [x, y]{Globals::instance().pointer_position()};
            return (x >= close_button_rect.left   - margin &&
                    x <= close_button_rect.right  + margin &&
                    y >= close_button_rect.top    - margin &&
                    y <= close_button_rect.bottom + margin);
        }};

    if (button == BTN_LEFT)
    {
        if (state == WL_POINTER_BUTTON_STATE_PRESSED && pointer_on_close_button())
        {
            pressed_close_button = true;
        }

        if (state == WL_POINTER_BUTTON_STATE_RELEASED)
        {
            if (pressed_close_button && pointer_on_close_button())
            {
                Globals::instance().close_window(static_cast<wl_surface*>(*this));
            }

            pressed_close_button = false;
        }
    }
}

void mfa::DecoratedXdgToplevelWindow::draw_new_content(Buffer* buffer)
{
    auto* mir_window{Globals::instance().window_for(static_cast<wl_surface*>(*this))};
    if (!mir_window) return;

    auto const x{config_.stroke_width / 2};
    auto const y{config_.stroke_width / 2};
    auto const width{buffer->width - (x * 2)};
    auto const height{buffer->height - (y * 2)};

    auto const close_button_scale{0.25};
    auto const pi{std::numbers::pi};
    auto const pi_2{pi / 2.0};

    cairo_set_source_rgba(buffer->cairo_context, 0, 0, 0, 0);
    cairo_paint(buffer->cairo_context);

    // Title bar
    {
        // Background
        auto const tbi{std::max(config_.title_bar_intensity - current_intensity_offset, 0.0)};
        cairo_set_source_rgba(buffer->cairo_context, tbi, tbi, tbi, alpha);
        cairo_set_line_width(buffer->cairo_context, config_.stroke_width);

        cairo_new_sub_path(buffer->cairo_context);
        cairo_arc(buffer->cairo_context, x, y + config_.title_bar_height, 0, 0, 0);
        cairo_arc(
            buffer->cairo_context,
            x + config_.title_bar_corner_radius,
            y + config_.title_bar_corner_radius,
            config_.title_bar_corner_radius,
            pi,
            -pi_2);
        cairo_arc(
            buffer->cairo_context,
            x + width - config_.title_bar_corner_radius,
            y + config_.title_bar_corner_radius,
            config_.title_bar_corner_radius,
            -pi_2,
            0);
        cairo_arc(buffer->cairo_context, x + width, y + config_.title_bar_height, 0, 0, 0);
        cairo_fill_preserve(buffer->cairo_context);

        auto const si{config_.stroke_intensity};
        cairo_set_source_rgba(buffer->cairo_context, si, si, si, 1);
        cairo_stroke(buffer->cairo_context);

        // Text
        cairo_set_source_rgb(buffer->cairo_context, 1, 1, 1);
        cairo_select_font_face(buffer->cairo_context, "Ubuntu", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(buffer->cairo_context, config_.title_bar_font_size);
        auto title_text{config_.title_bar_text + " - ID " + std::to_string(mir_window->id)};
        cairo_text_extents_t text_extents;
        cairo_text_extents(buffer->cairo_context, title_text.c_str(), &text_extents);
        cairo_move_to(
            buffer->cairo_context,
            (buffer->width - text_extents.width) / 2.0 - text_extents.x_bearing,
            y + (config_.title_bar_height - text_extents.height) / 2.0 - text_extents.y_bearing);
        cairo_show_text(buffer->cairo_context, title_text.c_str());

        // Close button
        auto const close_button_size{config_.title_bar_height * close_button_scale};
        auto const close_button_padding{(config_.title_bar_height - close_button_size) / 2.0};
        auto const left{x + width - close_button_padding - close_button_size};
        auto const top{y + close_button_padding};
        auto const right{left + close_button_size};
        auto const bottom{top + close_button_size};
        close_button_rect = {left, top, right, bottom};

        cairo_set_source_rgb(buffer->cairo_context, 1, 1, 1);
        cairo_set_line_width(buffer->cairo_context, 2);
        cairo_move_to(buffer->cairo_context, left, top);
        cairo_line_to(buffer->cairo_context, right, bottom);
        cairo_move_to(buffer->cairo_context, right, top);
        cairo_line_to(buffer->cairo_context, left, bottom);
        cairo_stroke(buffer->cairo_context);
    }

    // Client rectangle
    {
        auto const bi{config_.background_intensity};
        cairo_set_source_rgba(buffer->cairo_context, bi, bi, bi, alpha);
        cairo_set_line_width(buffer->cairo_context, config_.stroke_width);
        cairo_rectangle(
            buffer->cairo_context,
            x,
            y + config_.title_bar_height,
            width,
            height - config_.title_bar_height);
        cairo_fill_preserve(buffer->cairo_context);

        auto const si{config_.stroke_intensity};
        cairo_set_source_rgba(buffer->cairo_context, si, si, si, 1);
        cairo_stroke(buffer->cairo_context);
    }

    // Text
    if (mir_window->parent)
    {
        auto const font_size{14};
        cairo_set_source_rgb(buffer->cairo_context, 0.2, 0.2, 0.2);
        cairo_select_font_face(buffer->cairo_context, "Ubuntu", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(buffer->cairo_context, font_size);

        auto const padding{10};
        auto print_line{
            [&, y_pos{0.0}](std::string const& text, double line_spacing = 1.5) mutable
            {
                cairo_text_extents_t text_extents;
                cairo_text_extents(buffer->cairo_context, text.c_str(), &text_extents);
                if (y_pos == 0)
                {
                    y_pos = config().title_bar_height + padding - text_extents.y_bearing;
                }
                cairo_move_to(buffer->cairo_context, config().stroke_width + padding - text_extents.x_bearing, y_pos);
                cairo_show_text(buffer->cairo_context, text.c_str());
                y_pos += font_size * line_spacing;
            }};

        print_line("Parent ID: " + std::to_string(mir_window->parent->id));
    }
}

void mfa::DecoratedXdgToplevelWindow::show_activated()
{
    current_intensity_offset = intensity_offset;
    redraw();
}

void mfa::DecoratedXdgToplevelWindow::show_unactivated()
{
    current_intensity_offset = 0;
    redraw();
}
