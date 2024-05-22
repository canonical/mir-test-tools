#ifndef POPUP_WINDOW_H_
#define POPUP_WINDOW_H_

#include "decorated_xdg_popup_window.h"

namespace mir_flutter_app
{
class PopupWindow : public DecoratedXdgPopupWindow
{
public:
    PopupWindow(wl_surface* surface, int32_t width, int32_t height, xdg_positioner* positioner, xdg_surface* parent);
    ~PopupWindow() override = default;

protected:
    void handle_mouse_button(wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
        override;

    void draw_new_content(Buffer* buffer) override;

    PopupWindow(PopupWindow&&) = default;
    PopupWindow& operator=(PopupWindow&&) = default;

private:
    bool pressed_left_mouse_button{};

    PopupWindow(PopupWindow const&) = delete;
    PopupWindow& operator=(PopupWindow const&) = delete;
};
}

#endif // POPUP_WINDOW_H_