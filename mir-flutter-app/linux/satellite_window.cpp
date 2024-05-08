#include "satellite_window.h"
#include "globals.h"
#include "mir-shell.h"
#include "xdg-shell.h"

#include <linux/input-event-codes.h>

#include <iostream>

namespace mfa = mir_flutter_app;

mfa::SatelliteWindow::SatelliteWindow(
    wl_surface* surface,
    int32_t width,
    int32_t height,
    mir_positioner_v1* positioner,
    xdg_toplevel* parent) :
    DecoratedXdgToplevelWindow{surface, width, height,
        {
            .title_bar_text = "satellite",
            .title_bar_corner_radius = 8,
            .title_bar_height = 24,
            .title_bar_font_size = 13
        }},
    mir_satellite_surface{
        Globals::instance().mir_shell() ?
        mir_shell_v1_get_satellite_surface(Globals::instance().mir_shell(), surface, positioner) :
        nullptr}
{
    static mir_satellite_surface_v1_listener const satellite_listener{.repositioned = [](void* ctx, auto... args) {
        static_cast<SatelliteWindow*>(ctx)->handle_repositioned(args...);
    }};

    xdg_toplevel_set_title(static_cast<xdg_toplevel*>(*this), "satellite");
    xdg_toplevel_set_parent(static_cast<xdg_toplevel*>(*this), parent);

    if (mir_satellite_surface)
    {
        mir_satellite_surface_v1_add_listener(mir_satellite_surface, &satellite_listener, this);
    }
}

mfa::SatelliteWindow::~SatelliteWindow()
{
    if (mir_satellite_surface)
    {
        mir_satellite_surface_v1_destroy(mir_satellite_surface);
    }
}

void mfa::SatelliteWindow::handle_keyboard_key(
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

void mfa::SatelliteWindow::handle_repositioned(
    mir_satellite_surface_v1* /*mir_satellite_surface_v1*/,
    uint32_t /*token*/)
{
    std::cout << "Received repositioned event" << std::endl;
}
