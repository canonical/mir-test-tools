#ifndef XDG_POPUP_WINDOW_H_
#define XDG_POPUP_WINDOW_H_

#include "window.h"

struct xdg_popup;
struct xdg_positioner;
struct xdg_surface;

namespace mir_flutter_app
{
class XdgPopupWindow : public Window
{
public:
    XdgPopupWindow(
        wl_surface* surface,
        int32_t width,
        int32_t height,
        xdg_surface* parent,
        xdg_positioner* positioner);
    ~XdgPopupWindow() override;

    explicit operator xdg_surface*() const { return xdgsurface; }

    void handle_mouse_button(wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
        override;

protected:
    XdgPopupWindow(XdgPopupWindow&&) = default;
    XdgPopupWindow& operator=(XdgPopupWindow&&) = default;

private:
    xdg_surface* xdgsurface;
    xdg_popup* xdgpopup;

    int32_t pending_width{};
    int32_t pending_height{};

    void handle_xdg_surface_configure(xdg_surface* surface, uint32_t serial);
    void handle_xdg_popup_configure(xdg_popup* popup, int32_t x, int32_t y, int32_t width, int32_t height);

    virtual void show() = 0;

    XdgPopupWindow(XdgPopupWindow const&) = delete;
    XdgPopupWindow& operator=(XdgPopupWindow const&) = delete;
};
}

#endif // XDG_POPUP_WINDOW_H_