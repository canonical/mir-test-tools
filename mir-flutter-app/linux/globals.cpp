#include "globals.h"
#include "xdg_toplevel_window.h"
#include "xdg_popup_window.h"
#include "regular_window.h"
#include "floating_regular_window.h"
#include "dialog_window.h"
#include "satellite_window.h"
#include "popup_window.h"
#include "tip_window.h"
#include "mir_window.h"
#include "xdg-shell.h"
#include "mir-shell.h"

#include <iomanip>
#include <iostream>
#include <vector>

namespace mfa = mir_flutter_app;

void mfa::Globals::bind_interfaces(wl_display* wl_display)
{
    if (display_ || !wl_display)
    {
        return;
    }
    display_ = wl_display;

    static wl_registry_listener const registry_listener{
        .global = [](void* ctx, auto... args) { static_cast<Globals*>(ctx)->handle_wl_registry_global(args...); },
        .global_remove = [](auto...) {}};

    static xdg_wm_base_listener const shell_listener{
        .ping = [](void*, xdg_wm_base* shell, uint32_t serial) { xdg_wm_base_pong(shell, serial); }};

    static wl_pointer_listener const pointer_listener{
        .enter = [](void* ctx, auto... args) { static_cast<Globals*>(ctx)->handle_mouse_enter(args...); },
        .leave = [](void* ctx, auto... args) { static_cast<Globals*>(ctx)->handle_mouse_leave(args...); },
        .motion = [](void* ctx, auto... args) { static_cast<Globals*>(ctx)->handle_mouse_motion(args...); },
        .button = [](void* ctx, auto... args) { static_cast<Globals*>(ctx)->handle_mouse_button(args...); },
        .axis = [](auto...) {}};

    static wl_keyboard_listener const keyboard_listener{
        .keymap = [](auto...) {},
        .enter = [](void* self, auto... args) { static_cast<Globals*>(self)->handle_keyboard_enter(args...); },
        .leave = [](void* self, auto... args) { static_cast<Globals*>(self)->handle_keyboard_leave(args...); },
        .key = [](void* self, auto... args) { static_cast<Globals*>(self)->handle_keyboard_key(args...); },
        .modifiers = [](void* self, auto... args) { static_cast<Globals*>(self)->handle_keyboard_modifiers(args...); },
        .repeat_info = [](auto...) {}};


    wl_registry* const registry{wl_display_get_registry(display())};
    if (!registry)
    {
        std::cerr << "Failed to retrieve registry from the Wayland display connection.\n";
        std::abort();
    }

    wl_registry_add_listener(registry, &registry_listener, this);
    wl_display_roundtrip(display());

    bool failed_binding{};
    if (!output_)
    {
        std::cerr << "Failed to bind to wl_output.\n";
        failed_binding = true;
    }
    if (!seat_)
    {
        std::cerr << "Failed to bind to wl_seat.\n";
        failed_binding = true;
    }
    if (!shm_)
    {
        std::cerr << "Failed to bind to wl_shm.\n";
        failed_binding = true;
    }
    if (!wm_base_)
    {
        std::cerr << "Failed to bind to wm_base.\n";
        failed_binding = true;
    }
    if (!mir_shell_)
    {
        std::cerr << "Failed to bind to mir_shell.\n";
        failed_binding = true;
    }

    if (failed_binding)
    {
        std::abort();
    }

    xdg_wm_base_add_listener(wm_base(), &shell_listener, nullptr);
    wl_display_roundtrip(display());

    pointer = wl_seat_get_pointer(seat_);
    keyboard = wl_seat_get_keyboard(seat_);
    wl_keyboard_add_listener(keyboard, &keyboard_listener, this);
    wl_pointer_add_listener(pointer, &pointer_listener, this);
}

auto mfa::Globals::make_regular_window(MirWindow* window) -> std::unique_ptr<XdgToplevelWindow>
{
    register_window(window);

    return std::make_unique<RegularWindow>(window->surface, window->size.width, window->size.height);
}

auto mfa::Globals::make_floating_regular_window(MirWindow* window) -> std::unique_ptr<XdgToplevelWindow>
{
    register_window(window);

    return std::make_unique<FloatingRegularWindow>(window->surface, window->size.width, window->size.height);
}

auto mfa::Globals::make_satellite_window(MirWindow* window) -> std::unique_ptr<XdgToplevelWindow>
{
    register_window(window);

    if (!window->parent)
    {
        std::cerr << "Satellite window must have a parent.\n";
        std::abort();
    }

    auto* const positioner{mir_shell_v1_create_positioner(mir_shell_)};
    mir_positioner_v1_set_anchor_rect(
        positioner,
        window->positioner.anchor_rect.x,
        window->positioner.anchor_rect.y,
        window->positioner.anchor_rect.width,
        window->positioner.anchor_rect.height);
    mir_positioner_v1_set_anchor(positioner, window->positioner.anchor);
    mir_positioner_v1_set_gravity(positioner, window->positioner.gravity);
    mir_positioner_v1_set_offset(positioner, window->positioner.offset.dx, window->positioner.offset.dy);
    mir_positioner_v1_set_constraint_adjustment(positioner, window->positioner.constraint_adjustment);

    auto* const parent{static_cast<xdg_toplevel*>(
        *std::get<std::unique_ptr<mfa::XdgToplevelWindow>>(window->parent->window))};
    return std::make_unique<SatelliteWindow>(
        window->surface,
        window->size.width,
        window->size.height,
        positioner,
        parent);
}

auto mfa::Globals::make_popup_window(MirWindow* window) -> std::unique_ptr<XdgPopupWindow>
{
    register_window(window);

    if (!window->parent)
    {
        std::cerr << "Poup window must have a parent.\n";
        std::abort();
    }

    auto* const parent{std::holds_alternative<std::unique_ptr<mfa::XdgToplevelWindow>>(window->parent->window) ?
        static_cast<xdg_surface*>(*std::get<std::unique_ptr<mfa::XdgToplevelWindow>>(window->parent->window)) :
        static_cast<xdg_surface*>(*std::get<std::unique_ptr<mfa::XdgPopupWindow>>(window->parent->window))};

    auto* const positioner{xdg_wm_base_create_positioner(wm_base_)};
    xdg_positioner_set_size(positioner, window->size.width, window->size.height);
    xdg_positioner_set_anchor_rect(
        positioner,
        window->positioner.anchor_rect.x,
        window->positioner.anchor_rect.y,
        window->positioner.anchor_rect.width,
        window->positioner.anchor_rect.height);
    xdg_positioner_set_anchor(positioner, window->positioner.anchor);
    xdg_positioner_set_gravity(positioner, window->positioner.gravity);
    xdg_positioner_set_constraint_adjustment(positioner, window->positioner.constraint_adjustment);
    xdg_positioner_set_offset(positioner, window->positioner.offset.dx, window->positioner.offset.dy);

    return std::make_unique<PopupWindow>(
        window->surface,
        window->size.width,
        window->size.height,
        positioner,
        parent);
}

auto mfa::Globals::make_tip_window(MirWindow* window) -> std::unique_ptr<XdgPopupWindow>
{
    register_window(window);

    if (!window->parent)
    {
        std::cerr << "Tip window must have a parent.\n";
        std::abort();
    }

    auto* const parent{std::holds_alternative<std::unique_ptr<mfa::XdgToplevelWindow>>(window->parent->window) ?
        static_cast<xdg_surface*>(*std::get<std::unique_ptr<mfa::XdgToplevelWindow>>(window->parent->window)) :
        static_cast<xdg_surface*>(*std::get<std::unique_ptr<mfa::XdgPopupWindow>>(window->parent->window))};

    auto* const positioner{xdg_wm_base_create_positioner(wm_base_)};
    xdg_positioner_set_size(positioner, window->size.width, window->size.height);
    xdg_positioner_set_anchor_rect(
        positioner,
        window->positioner.anchor_rect.x,
        window->positioner.anchor_rect.y,
        window->positioner.anchor_rect.width,
        window->positioner.anchor_rect.height);
    xdg_positioner_set_anchor(positioner, window->positioner.anchor);
    xdg_positioner_set_gravity(positioner, window->positioner.gravity);
    xdg_positioner_set_constraint_adjustment(positioner, window->positioner.constraint_adjustment);
    xdg_positioner_set_offset(positioner, window->positioner.offset.dx, window->positioner.offset.dy);

    return std::make_unique<TipWindow>(
        window->surface,
        window->size.width,
        window->size.height,
        positioner,
        parent);
}

auto mfa::Globals::make_dialog_window(MirWindow* window) -> std::unique_ptr<XdgToplevelWindow>
{
    register_window(window);

    auto* const parent{window->parent ?
        static_cast<xdg_toplevel*>(*std::get<std::unique_ptr<mfa::XdgToplevelWindow>>(window->parent->window))  :
        nullptr};

    return std::make_unique<DialogWindow>(window->surface, window->size.width, window->size.height, parent);
}

void mfa::Globals::close_window(wl_surface* surface)
{
    auto* const mir_window{window_for(surface)};
    mir_flutter_app::Globals::instance().deregister_window(mir_window);

    std::vector<wl_surface*> children;
    for (auto const [_, window] : windows)
    {
        if (window->surface != surface && window->parent && window->parent->surface == surface)
        {
            children.push_back(window->surface);
        }
    }

    for (auto* const surface : children)
    {
        close_window(surface);
    }

    gtk_widget_destroy(GTK_WIDGET(mir_window));
}

void mfa::Globals::register_window(MirWindow* window) { windows[window->surface] = window; }

void mfa::Globals::deregister_window(MirWindow* window) { windows.erase(window->surface); }

auto mfa::Globals::window_for(wl_surface* surface) -> MirWindow*
{
    return windows.contains(surface) ? windows[surface] : nullptr;
}

bool mfa::Globals::is_registered(MirWindow* window) const { return window && windows.contains(window->surface); }

void mfa::Globals::handle_wl_registry_global(
    wl_registry* registry,
    uint32_t id,
    char const* interface,
    uint32_t version)
{
    std::string_view name{interface};
    bool bound{};

    if (!compositor_ && name == wl_compositor_interface.name && wl_compositor_interface.version >= 1)
    {
        version = std::min(version, 4u);
        compositor_ = static_cast<wl_compositor*>(wl_registry_bind(registry, id, &wl_compositor_interface, version));
        bound = true;
    }
    else if (!output_ && name == wl_output_interface.name && wl_output_interface.version >= 1)
    {
        version = std::min(version, 4u);
        output_ = static_cast<wl_output*>(wl_registry_bind(registry, id, &wl_output_interface, version));
        bound = true;
    }
    else if (!shm_ && name == wl_shm_interface.name && wl_shm_interface.version >= 1)
    {
        version = std::min(version, 1u);
        shm_ = static_cast<wl_shm*>(wl_registry_bind(registry, id, &wl_shm_interface, version));
        bound = true;
        // Normally we'd add a listener to pick up the supported formats here
        // As luck would have it, I know that argb8888 is the only format we support :)
    }
    else if (!seat_ && name == wl_seat_interface.name && wl_seat_interface.version >= 1)
    {
        version = std::min(version, 4u);
        seat_ = static_cast<wl_seat*>(wl_registry_bind(registry, id, &wl_seat_interface, version));
        bound = true;
    }
    else if (!mir_shell_ && name == mir_shell_v1_interface.name && mir_shell_v1_interface.version >= 1)
    {
        version = std::min(version, 1u);
        mir_shell_ =
            static_cast<mir_shell_v1*>(wl_registry_bind(registry, id, &mir_shell_v1_interface, std::min(version, 1u)));
        bound = true;
    }
    else if (!wm_base_ && name == xdg_wm_base_interface.name && xdg_wm_base_interface.version >= 1)
    {
        version = std::min(version, 1u);
        wm_base_ =
            static_cast<xdg_wm_base*>(wl_registry_bind(registry, id, &xdg_wm_base_interface, std::min(version, 1u)));
        bound = true;
    }

    if (bound)
    {
        std::cout << "Bound to " << name << " (v" << version << ")" << std::endl;
    }
}

void mfa::Globals::handle_mouse_enter(
    wl_pointer* /*pointer*/,
    uint32_t /*serial*/,
    wl_surface* surface,
    wl_fixed_t surface_x,
    wl_fixed_t surface_y)
{
    mouse_focus = window_for(surface);

    // if (mouse_focus)
    // {
    //     std::cout << "Window " << mouse_focus->id;
    // }
    // else
    // {
    //     std::cout << "Main window";
    // }
    // std::cout << " - ";

    // std::cout << "mouse_enter: (" << std::fixed << std::setprecision(2)
    //     << wl_fixed_to_double(surface_x) << ", " << wl_fixed_to_double(surface_y) << ")" << std::endl;
}

void mfa::Globals::handle_mouse_leave(wl_pointer* /*pointer*/, uint32_t /*serial*/, wl_surface* surface)
{
    // if (mouse_focus)
    // {
    //     std::cout << "Window " << mouse_focus->id;
    // }
    // else
    // {
    //     std::cout << "Main window";
    // }
    // std::cout << " - ";

    // std::cout << "mouse_leave" << std::endl;

    if (mouse_focus == window_for(surface))
    {
        mouse_focus = nullptr;
    }
}

void mfa::Globals::handle_mouse_motion(
    wl_pointer* /*pointer*/,
    uint32_t/*time*/,
    wl_fixed_t surface_x,
    wl_fixed_t surface_y)
{
    // if (mouse_focus)
    // {
    //     std::cout << "Window " << mouse_focus->id;
    // }
    // else
    // {
    //     std::cout << "Main window";
    // }
    // std::cout << " - ";

    // std::cout << "mouse_motion: (" << std::fixed << std::setprecision(2)
    //     << wl_fixed_to_double(surface_x) << ", " << wl_fixed_to_double(surface_y)
    //     << ") @ " << time << std::endl;

    pointer_position_ = {wl_fixed_to_double(surface_x), wl_fixed_to_double(surface_y)};
}

void mfa::Globals::handle_mouse_button(
    wl_pointer* pointer,
    uint32_t serial,
    uint32_t time,
    uint32_t button,
    uint32_t state)
{
    // if (mouse_focus)
    // {
    //     std::cout << "Window " << mouse_focus->id;
    // }
    // else
    // {
    //     std::cout << "Main window";
    // }
    // std::cout << " - ";

    // std::cout << "mouse_button: button " << button << ", state " << state
    //     << " @ " << time << std::endl;

    if (is_registered(mouse_focus))
    {
        if (std::holds_alternative<std::unique_ptr<mfa::XdgToplevelWindow>>(mouse_focus->window))
        {
            std::get<std::unique_ptr<mfa::XdgToplevelWindow>>(mouse_focus->window)->
                handle_mouse_button(pointer, serial, time, button, state);
        }
        else
        {
            std::get<std::unique_ptr<mfa::XdgPopupWindow>>(mouse_focus->window)->
                handle_mouse_button(pointer, serial, time, button, state);
        }
    }
}

void mfa::Globals::handle_keyboard_enter(wl_keyboard* /*keyboard*/, uint32_t, wl_surface* surface, wl_array*)
{
    keyboard_focus = window_for(surface);

    // if (keyboard_focus)
    // {
    //     std::cout << "Window " << keyboard_focus->id;
    // }
    // else
    // {
    //     std::cout << "Main window";
    // }
    // std::cout << " - ";

    // std::cout << "keyboard_enter" << std::endl;
}

void mfa::Globals::handle_keyboard_leave(wl_keyboard* /*keyboard*/, uint32_t, wl_surface* surface)
{
    // if (keyboard_focus)
    // {
    //     std::cout << "Window " << keyboard_focus->id;
    // }
    // else
    // {
    //     std::cout << "Main window";
    // }
    // std::cout << " - ";

    // std::cout << "keyboard_leave" << std::endl;

    if (keyboard_focus == window_for(surface))
    {
        keyboard_focus = nullptr;
    }
}

void mfa::Globals::handle_keyboard_key(
    wl_keyboard* keyboard,
    uint32_t serial,
    uint32_t time,
    uint32_t key,
    uint32_t state)
{
    // if (keyboard_focus)
    // {
    //     std::cout << "Window " << keyboard_focus->id;
    // }
    // else
    // {
    //     std::cout << "Main window";
    // }
    // std::cout << " - ";

    // std::cout << "keyboard_key: key " << key << ", state " << state << std::endl;

    if (is_registered(keyboard_focus))
    {
        if (std::holds_alternative<std::unique_ptr<mfa::XdgToplevelWindow>>(keyboard_focus->window))
        {
            std::get<std::unique_ptr<mfa::XdgToplevelWindow>>(keyboard_focus->window)->
                handle_keyboard_key(keyboard, serial, time, key, state);
        }
        else
        {
            std::get<std::unique_ptr<mfa::XdgPopupWindow>>(keyboard_focus->window)->
                handle_keyboard_key(keyboard, serial, time, key, state);
        }
    }
}

void mfa::Globals::handle_keyboard_modifiers(
    wl_keyboard* keyboard,
    uint32_t serial,
    uint32_t mods_depressed,
    uint32_t mods_latched,
    uint32_t mods_locked,
    uint32_t group)
{
    // if (keyboard_focus)
    // {
    //     std::cout << "Window " << keyboard_focus->id;
    // }
    // else
    // {
    //     std::cout << "Main window";
    // }
    // std::cout << " - ";

    // std::cout << "keyboard_modifiers: depressed " << mods_depressed
    //     << ", latched " << mods_latched
    //     << ", locked " << mods_locked
    //     << ", group " << group
    //     << std::endl;

    if (is_registered(keyboard_focus))
    {
        if (std::holds_alternative<std::unique_ptr<mfa::XdgToplevelWindow>>(keyboard_focus->window))
        {
            std::get<std::unique_ptr<mfa::XdgToplevelWindow>>(keyboard_focus->window)->
                handle_keyboard_modifiers(
                keyboard,
                serial,
                mods_depressed,
                mods_latched,
                mods_locked,
                group);
        }
        else
        {
            std::get<std::unique_ptr<mfa::XdgPopupWindow>>(keyboard_focus->window)->
                handle_keyboard_modifiers(
                keyboard,
                serial,
                mods_depressed,
                mods_latched,
                mods_locked,
                group);
        }
    }
}
