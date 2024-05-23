#include "floating_regular_window.h"
#include "globals.h"
#include "mir-shell.h"
#include "xdg-shell.h"

#include <linux/input-event-codes.h>

namespace mfa = mir_flutter_app;

mfa::FloatingRegularWindow::FloatingRegularWindow(wl_surface* surface, int32_t width, int32_t height) :
    DecoratedXdgToplevelWindow{surface, width, height, {.title_bar_text = "floating_regular"}},
    mir_floating_regular_surface{
        Globals::instance().mir_shell() ?
        mir_shell_v1_get_floating_regular_surface(Globals::instance().mir_shell(), surface) :
        nullptr}
{
    xdg_toplevel_set_title(static_cast<xdg_toplevel*>(*this), "floating_regular");
}

mfa::FloatingRegularWindow::~FloatingRegularWindow()
{
    if (mir_floating_regular_surface)
    {
        mir_floating_regular_surface_v1_destroy(mir_floating_regular_surface);
    }
}

void mfa::FloatingRegularWindow::handle_keyboard_key(
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
