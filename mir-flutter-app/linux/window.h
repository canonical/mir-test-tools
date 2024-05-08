#ifndef WINDOW_H_
#define WINDOW_H_

#include <array>
#include <cstdint>

struct wl_buffer;
struct wl_callback;
struct wl_keyboard;
struct wl_pointer;
struct wl_surface;

using cairo_surface_t = struct _cairo_surface;
using cairo_t = struct _cairo;

namespace mir_flutter_app
{
class Window
{
public:
    Window(wl_surface* surface, int32_t width, int32_t height);
    virtual ~Window();

    explicit operator wl_surface*() const { return surface; }

    auto width() const -> int32_t { return width_; }
    auto height() const -> int32_t { return height_; }

    virtual void handle_mouse_button(
        wl_pointer* pointer,
        uint32_t serial,
        uint32_t time,
        uint32_t button,
        uint32_t state) {};
    virtual void handle_keyboard_key(
        wl_keyboard* keyboard,
        uint32_t serial,
        uint32_t time,
        uint32_t key,
        uint32_t state) {};
    virtual void handle_keyboard_modifiers(
        wl_keyboard* keyboard,
        uint32_t serial,
        uint32_t mods_depressed,
        uint32_t mods_latched,
        uint32_t mods_locked,
        uint32_t group) {};

protected:
    static int const pixel_size{4};

    struct Buffer
    {
        wl_buffer* buffer{};
        bool available{};
        int width{};
        int height{};
        void* content_area{};

        cairo_surface_t* cairo_surface;
        cairo_t* cairo_context;
    };

    void redraw();
    void resize(int32_t width, int32_t height);

    Window(Window&&) = default;
    Window& operator=(Window&&) = default;

private:
    static int const num_buffers{2};

    wl_surface* surface;
    int width_;
    int height_;

    std::array<Buffer, Window::num_buffers> buffers{};
    bool need_to_draw{true};

    void handle_frame_callback(wl_callback* callback, uint32_t time);

    void update_free_buffers(wl_buffer* buffer);
    void prepare_buffer(Buffer& b);
    auto find_free_buffer() -> Buffer*;

    virtual void draw_new_content(Buffer* buffer) = 0;

    Window(Window const&) = delete;
    Window& operator=(Window const&) = delete;
};
}

#endif // WINDOW_H_