#include "xdg_popup_window.h"
#include "globals.h"
#include "mir_window.h"
#include "xdg-shell.h"

#include <iostream>

namespace mfa = mir_flutter_app;

mfa::XdgPopupWindow::XdgPopupWindow(
    wl_surface* surface,
    int32_t width,
    int32_t height,
    xdg_surface* parent,
    xdg_positioner* positioner) :
    Window{surface, width, height},
    xdgsurface{xdg_wm_base_get_xdg_surface(Globals::instance().wm_base(), static_cast<wl_surface*>(*this))},
    xdgpopup{xdg_surface_get_popup(xdgsurface, parent, positioner)}
{
    static xdg_popup_listener const shell_popup_listener{
        .configure = [](void* ctx, auto... args)
            {
                static_cast<XdgPopupWindow*>(ctx)->handle_xdg_popup_configure(args...);
            },
        .popup_done = [](auto...) {},
        .repositioned = [](auto...) {}};
    static xdg_surface_listener const shell_surface_listener{.configure = [](void* ctx, auto... args) {
        static_cast<XdgPopupWindow*>(ctx)->handle_xdg_surface_configure(args...);
    }};

    xdg_popup_add_listener(xdgpopup, &shell_popup_listener, this);
    xdg_surface_add_listener(xdgsurface, &shell_surface_listener, this);
}

mfa::XdgPopupWindow::~XdgPopupWindow()
{
    xdg_popup_destroy(xdgpopup);
    xdg_surface_destroy(xdgsurface);
}

void mfa::XdgPopupWindow::handle_mouse_button(
    wl_pointer* pointer,
    uint32_t serial,
    uint32_t time,
    uint32_t button,
    uint32_t state)
{
    Window::handle_mouse_button(pointer, serial, time, button, state);
}

void mfa::XdgPopupWindow::handle_xdg_surface_configure(xdg_surface* surface, uint32_t serial)
{
    auto* window{Globals::instance().window_for(static_cast<wl_surface*>(*this))};
    std::cout << "Window " << window->id << " - ";

    std::cout << "Received xdg_surface_configure" << std::endl;

    resize(pending_width, pending_height);

    show();

    xdg_surface_ack_configure(surface, serial);
}

void mfa::XdgPopupWindow::handle_xdg_popup_configure(
    xdg_popup* /*popup*/,
    int32_t x,
    int32_t y,
    int32_t width,
    int32_t height)
{
    auto* window{Globals::instance().window_for(static_cast<wl_surface*>(*this))};
    std::cout << "Window " << window->id << " - ";

    std::cout << "Received xdg_popup_configure: x: "
        << x << ", y: " << y << ", width " << width << ", height " << height << std::endl;

    pending_width = width;
    pending_height = height;
}
