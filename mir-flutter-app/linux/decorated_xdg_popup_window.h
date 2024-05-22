#ifndef XDG_DECORATED_POPUP_WINDOW_H_
#define XDG_DECORATED_POPUP_WINDOW_H_

#include "xdg_popup_window.h"

namespace mir_flutter_app
{
class DecoratedXdgPopupWindow : public XdgPopupWindow
{
public:
    struct Configuration
    {
        double background_intensity{0.95};
        double corner_radius{8.0};

        double stroke_width{1.0};
        double stroke_intensity{0.2};
    };

    DecoratedXdgPopupWindow(
        wl_surface* surface,
        int32_t width,
        int32_t height,
        xdg_surface* parent,
        xdg_positioner* positioner,
        Configuration config);

    auto config() -> Configuration const& { return config_; }

protected:
    void draw_new_content(Buffer* buffer) override;

private:
    Configuration config_;

    double alpha{1};
    double intensity_offset{0.1};

    void show() override;
};
}

#endif // XDG_DECORATED_POPUP_WINDOW_H_
