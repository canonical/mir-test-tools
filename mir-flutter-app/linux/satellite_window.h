#ifndef SATELLITE_WINDOW_H_
#define SATELLITE_WINDOW_H_

#include "decorated_xdg_toplevel_window.h"

struct mir_positioner_v1;
struct mir_satellite_surface_v1;
struct xdg_toplevel;

namespace mir_flutter_app
{
class SatelliteWindow : public DecoratedXdgToplevelWindow
{
public:
    SatelliteWindow(
        wl_surface* surface,
        int32_t width,
        int32_t height,
        mir_positioner_v1* positioner,
        xdg_toplevel* parent);
    ~SatelliteWindow() override;

protected:
    void handle_keyboard_key(wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
        override;

    SatelliteWindow(SatelliteWindow&&) = default;
    SatelliteWindow& operator=(SatelliteWindow&&) = default;

private:
    mir_satellite_surface_v1* mir_satellite_surface;

    void handle_repositioned(mir_satellite_surface_v1* mir_satellite_surface_v1, uint32_t token);

    SatelliteWindow(SatelliteWindow const&) = delete;
    SatelliteWindow& operator=(SatelliteWindow const&) = delete;
};
}

#endif // SATELLITE_WINDOW_H_