#ifndef MIR_WINDOW_H_
#define MIR_WINDOW_H_

#include <gtk/gtk.h>

#include "mir-shell.h"

#include <memory>
#include <set>
#include <variant>

namespace mir_flutter_app
{
class XdgToplevelWindow;
class XdgPopupWindow;
}

enum class MirWindowArchetype
{
    regular,
    floating_regular,
    dialog,
    satellite,
    popup,
    tip
};

struct MirWindowSize
{
    int32_t width;
    int32_t height;
};

struct MirWindowRect
{
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
};

struct MirWindowOffset
{
    int32_t dx;
    int32_t dy;
};

struct MirWindowPositioner
{
    MirWindowRect anchor_rect;
    mir_positioner_v1_anchor anchor;
    mir_positioner_v1_gravity gravity;
    MirWindowOffset offset;
    uint32_t constraint_adjustment;
};

G_DECLARE_FINAL_TYPE(MirWindow, mir_window, MIR, WINDOW, GtkWindow)

struct _MirWindow
{
    GtkWindow parent_instance;

    int id;

    MirWindow* parent;
    std::set<MirWindow*> children;
    MirWindowArchetype archetype;

    wl_surface* surface;
    MirWindowSize size;
    MirWindowPositioner positioner;

    std::variant
        <
            std::unique_ptr<mir_flutter_app::XdgToplevelWindow>,
            std::unique_ptr<mir_flutter_app::XdgPopupWindow>
        > window;
};

#endif // MIR_WINDOW_H_
