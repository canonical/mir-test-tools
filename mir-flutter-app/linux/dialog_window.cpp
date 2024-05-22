#include "dialog_window.h"
#include "globals.h"
#include "mir-shell.h"
#include "xdg-shell.h"

#include <linux/input-event-codes.h>

namespace mfa = mir_flutter_app;

mfa::DialogWindow::DialogWindow(wl_surface* surface, int32_t width, int32_t height, xdg_toplevel* parent) :
    DecoratedXdgToplevelWindow{surface, width, height, {.background_intensity = 0.95, .title_bar_text = "dialog"}},
    mir_dialog_surface{
        Globals::instance().mir_shell() ?
        mir_shell_v1_get_dialog_surface(Globals::instance().mir_shell(), surface) :
        nullptr}
{
    xdg_toplevel_set_title(static_cast<xdg_toplevel*>(*this), "dialog");
    xdg_toplevel_set_parent(static_cast<xdg_toplevel*>(*this), parent);
}

mfa::DialogWindow::~DialogWindow()
{
    if (mir_dialog_surface)
    {
        mir_dialog_surface_v1_destroy(mir_dialog_surface);
    }
}

void mfa::DialogWindow::handle_keyboard_key(
    wl_keyboard* keyboard,
    uint32_t serial,
    uint32_t time,
    uint32_t key,
    uint32_t state)
{
    DecoratedXdgToplevelWindow::handle_keyboard_key(keyboard, serial, time, key, state);

    if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
    {
        if (key == KEY_ESC)
        {
            Globals::instance().close_window(static_cast<wl_surface*>(*this));
        }
    }
}
