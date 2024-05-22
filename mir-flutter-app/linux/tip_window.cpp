#include "tip_window.h"

#include <cairo.h>

namespace mfa = mir_flutter_app;

mfa::TipWindow::TipWindow(
    wl_surface* surface,
    int32_t width,
    int32_t height,
    xdg_positioner* positioner,
    xdg_surface* parent) :
    DecoratedXdgPopupWindow{surface, width, height, parent, positioner, {}}
{
}

void mfa::TipWindow::draw_new_content(Buffer* buffer)
{
    DecoratedXdgPopupWindow::draw_new_content(buffer);

    auto const* text{"Tip"};
    cairo_set_source_rgb(buffer->cairo_context, 0.2, 0.2, 0.2);
    cairo_select_font_face(buffer->cairo_context, "Ubuntu", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(buffer->cairo_context, 24);

    cairo_text_extents_t text_extents;
    cairo_text_extents(buffer->cairo_context, text, &text_extents);

    cairo_move_to(
        buffer->cairo_context,
        (buffer->width - text_extents.width) / 2.0 - text_extents.x_bearing,
        (buffer->height - text_extents.height) / 2.0 - text_extents.y_bearing);

    cairo_show_text(buffer->cairo_context, text);
}
