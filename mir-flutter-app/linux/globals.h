#ifndef GLOBALS_H_
#define GLOBALS_H_

#include <cstdint>
#include <map>
#include <memory>

struct wl_array;
struct wl_compositor;
struct wl_display;
struct wl_keyboard;
struct wl_output;
struct wl_pointer;
struct wl_registry;
struct wl_registry_listener;
struct wl_seat;
struct wl_surface;
struct wl_shm;
struct xdg_wm_base;
struct xdg_wm_base_listener;

struct mir_positioner_v1;
struct mir_shell_v1;

using MirWindow = struct _MirWindow;
using wl_fixed_t = int32_t;

namespace mir_flutter_app
{
class XdgPopupWindow;
class XdgToplevelWindow;

class Globals
{
public:
    Globals(Globals const&) = delete;
    Globals(Globals&&) = delete;
    Globals& operator=(Globals const&) = delete;
    Globals& operator=(Globals&&) = delete;
    ~Globals() = default;

    static Globals& instance()
    {
        static Globals instance;
        return instance;
    }

    void bind_interfaces(wl_display* wl_display);
    void init_cairo();

    auto compositor() const -> wl_compositor* { return compositor_; }
    auto display() const -> wl_display* { return display_; }
    auto output() const -> wl_output* { return output_; }
    auto seat() const -> wl_seat* { return seat_; }
    auto shm() const -> wl_shm* { return shm_; }
    auto wm_base() const -> xdg_wm_base* { return wm_base_; }
    auto mir_shell() const -> mir_shell_v1* { return mir_shell_; }

    auto make_regular_window(MirWindow* window) -> std::unique_ptr<XdgToplevelWindow>;
    auto make_floating_regular_window(MirWindow* window) -> std::unique_ptr<XdgToplevelWindow>;
    auto make_dialog_window(MirWindow* window) -> std::unique_ptr<XdgToplevelWindow>;
    auto make_satellite_window(MirWindow* window) -> std::unique_ptr<XdgToplevelWindow>;
    auto make_popup_window(MirWindow* window) -> std::unique_ptr<XdgPopupWindow>;
    auto make_tip_window(MirWindow* window) -> std::unique_ptr<XdgPopupWindow>;
    void close_window(wl_surface* surface);

    auto window_for(wl_surface* surface) -> MirWindow*;
    auto pointer_position() -> std::tuple<double, double> { return pointer_position_; }

private:
    void register_window(MirWindow* window);
    void deregister_window(MirWindow* window);
    auto is_registered(MirWindow* window) const -> bool;

    void handle_wl_registry_global(wl_registry* registry, uint32_t id, char const* interface, uint32_t version);

    void handle_mouse_button(wl_pointer*, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
    void handle_keyboard_key(wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
    void handle_keyboard_modifiers(
        wl_keyboard* keyboard,
        uint32_t serial,
        uint32_t mods_depressed,
        uint32_t mods_latched,
        uint32_t mods_locked,
        uint32_t group);
    void handle_mouse_enter(
        wl_pointer* pointer,
        uint32_t serial,
        wl_surface* surface,
        wl_fixed_t surface_x,
        wl_fixed_t surface_y);
    void handle_mouse_leave(wl_pointer* pointer, uint32_t serial, wl_surface* surface);
    void handle_mouse_motion(wl_pointer* pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y);

    void handle_keyboard_enter(wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array* keys);
    void handle_keyboard_leave(wl_keyboard* keyboard, uint32_t serial, wl_surface* surface);

    wl_compositor* compositor_{};
    wl_display* display_{};
    wl_output* output_{};
    wl_seat* seat_{};
    wl_shm* shm_{};
    xdg_wm_base* wm_base_{};
    mir_shell_v1* mir_shell_{};

    wl_pointer* pointer{};
    wl_keyboard* keyboard{};
    MirWindow* mouse_focus{};
    MirWindow* keyboard_focus{};

    std::map<wl_surface*, MirWindow*> windows;

    std::tuple<double, double> pointer_position_;

    Globals() = default;
};
}

#endif // GLOBALS_H_
