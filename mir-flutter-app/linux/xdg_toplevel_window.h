#ifndef XDG_TOPLEVEL_WINDOW_H_
#define XDG_TOPLEVEL_WINDOW_H_

#include "window.h"

struct wl_array;
struct xdg_surface;
struct xdg_toplevel;

namespace mir_flutter_app
{
class XdgToplevelWindow : public Window
{
public:
    XdgToplevelWindow(wl_surface* surface, int32_t width, int32_t height);
    ~XdgToplevelWindow() override;

    explicit operator xdg_surface*() const { return xdgsurface; }
    explicit operator xdg_toplevel*() const { return xdgtoplevel; }

    void handle_mouse_button(wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
        override;

protected:
    XdgToplevelWindow(XdgToplevelWindow&&) = default;
    XdgToplevelWindow& operator=(XdgToplevelWindow&&) = default;

private:
    xdg_surface* xdgsurface;
    xdg_toplevel* xdgtoplevel;

    bool is_activated{};
    int32_t pending_width{};
    int32_t pending_height{};

    void handle_xdg_surface_configure(xdg_surface* surface, uint32_t serial);
    void handle_xdg_toplevel_configure(xdg_toplevel* toplevel, int32_t width, int32_t height, wl_array* states);

    virtual void show_activated() = 0;
    virtual void show_unactivated() = 0;

    XdgToplevelWindow(XdgToplevelWindow const&) = delete;
    XdgToplevelWindow& operator=(XdgToplevelWindow const&) = delete;
};
}

#endif // XDG_TOPLEVEL_WINDOW_H_