#include "decorated_xdg_popup_window.h"
#include "globals.h"
#include "mir_window.h"

#include <cairo.h>
#include <linux/input-event-codes.h>

#include <numbers>

namespace mfa = mir_flutter_app;

mfa::DecoratedXdgPopupWindow::DecoratedXdgPopupWindow(
    wl_surface* surface,
    int32_t width,
    int32_t height,
    xdg_surface* parent,
    xdg_positioner* positioner,
    Configuration config) :
    XdgPopupWindow(surface, width, height, parent, positioner),
    config_{config}
{
    show();
}

void mfa::DecoratedXdgPopupWindow::draw_new_content(Buffer* buffer)
{
    auto* mir_window{Globals::instance().window_for(static_cast<wl_surface*>(*this))};
    if (!mir_window) return;

    auto const x{config_.stroke_width / 2};
    auto const y{config_.stroke_width / 2};
    auto const width{buffer->width - (x * 2)};
    auto const height{buffer->height - (y * 2)};

    cairo_set_source_rgba(buffer->cairo_context, 0, 0, 0, 0);
    cairo_paint(buffer->cairo_context);

    // Background
    {
        auto const pi{std::numbers::pi};
        auto const pi_2{pi / 2.0};

        auto const tbi{std::max(config_.background_intensity - intensity_offset, 0.0)};
        cairo_set_source_rgba(buffer->cairo_context, tbi, tbi, tbi, alpha);
        cairo_set_line_width(buffer->cairo_context, config_.stroke_width);

        cairo_new_sub_path(buffer->cairo_context);
        // Top-left corner
        cairo_arc(
            buffer->cairo_context,
            x + config_.corner_radius,
            y + config_.corner_radius,
            config_.corner_radius,
            pi,
            -pi_2);
        // Top-right corner
        cairo_arc(
            buffer->cairo_context,
            x + width - config_.corner_radius,
            y + config_.corner_radius,
            config_.corner_radius,
            -pi_2,
            0);
        // Bottom-right corner
        cairo_arc(
            buffer->cairo_context,
            x + width - config_.corner_radius,
            y + height - config_.corner_radius,
            config_.corner_radius,
            0,
            pi_2);
        // Bottom-left corner
        cairo_arc(
            buffer->cairo_context,
            x + config_.corner_radius,
            y + height - config_.corner_radius,
            config_.corner_radius,
            pi_2,
            pi);
        cairo_close_path(buffer->cairo_context);
        cairo_fill_preserve(buffer->cairo_context);

        auto const si{config_.stroke_intensity};
        cairo_set_source_rgba(buffer->cairo_context, si, si, si, 1);
        cairo_stroke(buffer->cairo_context);
    }

    // Text
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
                    y_pos = padding - text_extents.y_bearing;
                }
                cairo_move_to(buffer->cairo_context, config().stroke_width + padding - text_extents.x_bearing, y_pos);
                cairo_show_text(buffer->cairo_context, text.c_str());
                y_pos += font_size * line_spacing;
            }};

        print_line("ID: " + std::to_string(mir_window->id) + " - Parent ID: " + std::to_string(mir_window->parent->id));
    }
}

void mfa::DecoratedXdgPopupWindow::show()
{
    redraw();
}
