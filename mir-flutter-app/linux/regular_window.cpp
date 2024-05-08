#include "regular_window.h"
#include "globals.h"
#include "mir-shell.h"
#include "xdg-shell.h"

#include <cairo.h>
#include <X11/X.h>
#include <linux/input-event-codes.h>

namespace mfa = mir_flutter_app;

mfa::RegularWindow::RegularWindow(wl_surface* surface, int32_t width, int32_t height) :
    DecoratedXdgToplevelWindow{surface, width, height, {.title_bar_text = "regular"}},
    mir_regular_surface{
        Globals::instance().mir_shell() ?
        mir_shell_v1_get_regular_surface(Globals::instance().mir_shell(), surface) :
        nullptr}
{
    xdg_toplevel_set_title(static_cast<xdg_toplevel*>(*this), "regular");
}

mfa::RegularWindow::~RegularWindow()
{
    if (mir_regular_surface)
    {
        mir_regular_surface_v1_destroy(mir_regular_surface);
    }
}

void mfa::RegularWindow::handle_keyboard_key(
    wl_keyboard* keyboard,
    uint32_t serial,
    uint32_t time,
    uint32_t key,
    uint32_t state)
{
    DecoratedXdgToplevelWindow::handle_keyboard_key(keyboard, serial, time, key, state);

    if (modifiers == ControlMask && state == WL_KEYBOARD_KEY_STATE_RELEASED)
    {
        switch (key)
        {
        case KEY_Q:
            Globals::instance().close_window(static_cast<wl_surface*>(*this));
            break;
        }
    }

    if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
    {
        switch (key)
        {
        case KEY_ESC:
            Globals::instance().close_window(static_cast<wl_surface*>(*this));
            break;
        }
    }
}

void mfa::RegularWindow::handle_keyboard_modifiers(
    wl_keyboard* keyboard,
    uint32_t serial,
    uint32_t mods_depressed,
    uint32_t mods_latched,
    uint32_t mods_locked,
    uint32_t group)
{
    DecoratedXdgToplevelWindow::handle_keyboard_modifiers(keyboard, serial, mods_depressed, mods_latched, mods_locked, group);

    modifiers = mods_depressed;
}

void mfa::RegularWindow::draw_new_content(Buffer* buffer)
{
    DecoratedXdgToplevelWindow::draw_new_content(buffer);

    std::string text{"Hello, Mir Shell!"};
    cairo_set_source_rgb(buffer->cairo_context, 0.2, 0.2, 0.2);
    cairo_select_font_face(buffer->cairo_context, "Ubuntu", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(buffer->cairo_context, 24);

    cairo_text_extents_t text_extents;
    cairo_text_extents(buffer->cairo_context, text.c_str(), &text_extents);

    cairo_move_to(
        buffer->cairo_context,
        (buffer->width - text_extents.width) / 2.0 - text_extents.x_bearing,
        (config().title_bar_height + buffer->height - text_extents.height) / 2.0 - text_extents.y_bearing);

    cairo_show_text(buffer->cairo_context, text.c_str());
}
