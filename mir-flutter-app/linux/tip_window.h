#ifndef TIP_WINDOW_H_
#define TIP_WINDOW_H_

#include "decorated_xdg_popup_window.h"

namespace mir_flutter_app
{
class TipWindow : public DecoratedXdgPopupWindow
{
public:
    TipWindow(wl_surface* surface, int32_t width, int32_t height, xdg_positioner* positioner, xdg_surface* parent);
    ~TipWindow() override = default;

protected:
    void draw_new_content(Buffer* buffer) override;

    TipWindow(TipWindow&&) = default;
    TipWindow& operator=(TipWindow&&) = default;

private:
    TipWindow(TipWindow const&) = delete;
    TipWindow& operator=(TipWindow const&) = delete;
};
}

#endif // TIP_WINDOW_H_