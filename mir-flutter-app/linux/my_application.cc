#include "my_application.h"
#include "globals.h"

#include <flutter_linux/flutter_linux.h>
#include <gdk/gdkwayland.h>

#include "flutter/generated_plugin_registrant.h"

#include "mir-shell.h"
#include "mir_window.h"
#include "xdg_toplevel_window.h"
#include "xdg_popup_window.h"

#include <iostream>

namespace
{
gchar const* const CHANNEL{"io.mir-server/window"};
int const MAIN_WINDOW_WIDTH{760};
int const MAIN_WINDOW_HEIGHT{580};
}

namespace mfa = mir_flutter_app;

struct _MyApplication
{
    GtkApplication parent_instance;
    char** dart_entrypoint_arguments;

    FlMethodChannel* mir_window_channel;

    GtkWindow* main_window;

    std::map<int, MirWindow*> windows;
};

G_DEFINE_TYPE(MyApplication, my_application, GTK_TYPE_APPLICATION)

G_DEFINE_TYPE(MirWindow, mir_window, GTK_TYPE_WINDOW)

template<FlValueType From, typename To>
To arg(FlValue* args, size_t index)
{
    static_assert(From == FL_VALUE_TYPE_FLOAT || From == FL_VALUE_TYPE_INT, "Unsupported Flutter type.");

    if constexpr(From == FL_VALUE_TYPE_FLOAT)
    {
        return static_cast<To>(fl_value_get_float(fl_value_get_list_value(args, index)));
    }
    else if constexpr(From == FL_VALUE_TYPE_INT)
    {
        return static_cast<To>(fl_value_get_int(fl_value_get_list_value(args, index)));
    }
}

static void mir_window_show(GtkWidget* widget)
{
    gtk_window_set_decorated(GTK_WINDOW(widget), FALSE);
    GTK_WIDGET_CLASS(mir_window_parent_class)->show(widget);
}

static void mir_window_realize(GtkWidget* widget)
{
    GTK_WIDGET_CLASS(mir_window_parent_class)->realize(widget);
    GdkWindow* const gdk_window{gtk_widget_get_window(widget)};
    gdk_wayland_window_set_use_custom_surface(gdk_window);
    g_return_if_fail(GDK_IS_WAYLAND_WINDOW(gdk_window) == true);
}

static void mir_window_map(GtkWidget* widget)
{
    MirWindow* const self{MIR_WINDOW(widget)};

    gtk_widget_set_size_request(widget, self->size.width, self->size.height);

    GdkWindow* const gdk_window{gtk_widget_get_window(widget)};
    self->surface = gdk_wayland_window_get_wl_surface(gdk_window);
    if (self->archetype == MirWindowArchetype::regular)
    {
        self->window = mfa::Globals::instance().make_regular_window(self);
    }
    if (self->archetype == MirWindowArchetype::floating_regular)
    {
        self->window = mfa::Globals::instance().make_floating_regular_window(self);
    }
    if (self->archetype == MirWindowArchetype::dialog)
    {
        self->window = mfa::Globals::instance().make_dialog_window(self);
    }
    if (self->archetype == MirWindowArchetype::satellite)
    {
        self->window = mfa::Globals::instance().make_satellite_window(self);
    }
    if (self->archetype == MirWindowArchetype::popup)
    {
        self->window = mfa::Globals::instance().make_popup_window(self);
    }
    if (self->archetype == MirWindowArchetype::tip)
    {
        self->window = mfa::Globals::instance().make_tip_window(self);
    }
}

static void method_response_cb(GObject* object, GAsyncResult* result, gpointer /*user_data*/)
{
    g_autoptr(GError) error{nullptr};
    g_autoptr(FlMethodResponse) response{
        fl_method_channel_invoke_method_finish(FL_METHOD_CHANNEL(object), result, &error)};
    if (!response)
    {
        g_warning("Failed to call method: %s", error->message);
        return;
    }

    fl_method_response_get_result(response, &error);
    if (!response)
    {
        g_warning("Method returned error: %s", error->message);
        return;
    }
}

static void mir_window_destroy(GtkWidget* widget)
{
    MirWindow* const self{MIR_WINDOW(widget)};

    if (self->parent)
    {
        self->parent->children.erase(self);
    }

    if (MyApplication* const application{MY_APPLICATION(gtk_window_get_application(GTK_WINDOW(self)))})
    {
        g_autoptr(FlValue) args{fl_value_new_int(self->id)};
        fl_method_channel_invoke_method(
            application->mir_window_channel,
            "onWindowClosed",
            args,
            nullptr,
            method_response_cb,
            nullptr);

        if (application->windows.contains(self->id))
        {
            application->windows.erase(self->id);
        }
        else
        {
            std::cerr << "Could not find window id in the window map.\n";
            std::abort();
        }
    }

    GTK_WIDGET_CLASS(mir_window_parent_class)->destroy(widget);
}

static void mir_window_class_init(MirWindowClass* klass)
{
    GTK_WIDGET_CLASS(klass)->show = mir_window_show;
    GTK_WIDGET_CLASS(klass)->realize = mir_window_realize;
    GTK_WIDGET_CLASS(klass)->map = mir_window_map;
    GTK_WIDGET_CLASS(klass)->destroy = mir_window_destroy;
}

static void mir_window_init(MirWindow* self) {}

static MirWindow* mir_window_new(
    MirWindowArchetype archetype,
    MirWindowSize size,
    MirWindowPositioner positioner,
    MirWindow* parent, int id)
{
    MirWindow* const self{MIR_WINDOW(g_object_new(mir_window_get_type(), nullptr))};
    self->archetype = archetype;
    self->size = size;
    self->positioner = positioner;
    self->parent = parent;
    self->children = {};
    self->id = id;

    if (self->parent)
    {
        self->parent->children.insert(self);
    }

    return self;
}

static void mir_window_method_cb(FlMethodChannel* /*channel*/, FlMethodCall* method_call, gpointer user_data)
{
    MyApplication* const self{MY_APPLICATION(user_data)};

    auto const get_new_window_id{[](const std::map<int, MirWindow*>& windows)
        {
            auto new_id{0};
            for (auto const& [id, _] : windows)
            {
                if (id == new_id)
                {
                    ++new_id;
                }
                else
                {
                    break;
                }
            }
            return new_id;
        }};

    std::string_view const name{fl_method_call_get_name(method_call)};
    if (name == "createRegularWindow")
    {
        FlValue* const args{fl_method_call_get_args(method_call)};
        if (fl_value_get_type(args) != FL_VALUE_TYPE_LIST || fl_value_get_length(args) != 2 ||
            fl_value_get_type(fl_value_get_list_value(args, 0)) != FL_VALUE_TYPE_FLOAT ||
            fl_value_get_type(fl_value_get_list_value(args, 1)) != FL_VALUE_TYPE_FLOAT)
        {
            fl_method_call_respond_error(method_call, "Bad Arguments", "", nullptr, nullptr);
            return;
        }

        MirWindowSize const size{
            .width = arg<FL_VALUE_TYPE_FLOAT, int>(args, 0),
            .height = arg<FL_VALUE_TYPE_FLOAT, int>(args, 1)};

        auto const new_id{get_new_window_id(self->windows)};
        MirWindow* const mir_window{mir_window_new(MirWindowArchetype::regular, size, {}, nullptr, new_id)};
        self->windows[mir_window->id] = mir_window;
        gtk_window_set_application(GTK_WINDOW(mir_window), GTK_APPLICATION(self));
        gtk_widget_show(GTK_WIDGET(mir_window));

        g_autoptr(FlValue) result{fl_value_new_int(mir_window->id)};
        fl_method_call_respond_success(method_call, result, nullptr);
    }
    else if (name == "createFloatingRegularWindow")
    {
        FlValue* const args{fl_method_call_get_args(method_call)};
        if (fl_value_get_type(args) != FL_VALUE_TYPE_LIST || fl_value_get_length(args) != 2 ||
            fl_value_get_type(fl_value_get_list_value(args, 0)) != FL_VALUE_TYPE_FLOAT ||
            fl_value_get_type(fl_value_get_list_value(args, 1)) != FL_VALUE_TYPE_FLOAT)
        {
            fl_method_call_respond_error(method_call, "Bad Arguments", "", nullptr, nullptr);
            return;
        }
        MirWindowSize const size{
            .width = arg<FL_VALUE_TYPE_FLOAT, int>(args, 0),
            .height = arg<FL_VALUE_TYPE_FLOAT, int>(args, 1)};

        auto const new_id{get_new_window_id(self->windows)};
        MirWindow* const mir_window{mir_window_new(MirWindowArchetype::floating_regular, size, {}, nullptr, new_id)};
        self->windows[mir_window->id] = mir_window;
        gtk_window_set_application(GTK_WINDOW(mir_window), GTK_APPLICATION(self));
        gtk_widget_show(GTK_WIDGET(mir_window));

        g_autoptr(FlValue) result{fl_value_new_int(mir_window->id)};
        fl_method_call_respond_success(method_call, result, nullptr);
    }
    else if (name == "createSatelliteWindow" ||
        name == "createPopupWindow" ||
        name == "createTipWindow")
    {
        FlValue* const args{fl_method_call_get_args(method_call)};
        if (fl_value_get_type(args) != FL_VALUE_TYPE_LIST || fl_value_get_length(args) != 12 ||
            fl_value_get_type(fl_value_get_list_value(args, 0)) != FL_VALUE_TYPE_INT ||
            fl_value_get_type(fl_value_get_list_value(args, 1)) != FL_VALUE_TYPE_FLOAT ||
            fl_value_get_type(fl_value_get_list_value(args, 2)) != FL_VALUE_TYPE_FLOAT ||
            fl_value_get_type(fl_value_get_list_value(args, 3)) != FL_VALUE_TYPE_FLOAT ||
            fl_value_get_type(fl_value_get_list_value(args, 4)) != FL_VALUE_TYPE_FLOAT ||
            fl_value_get_type(fl_value_get_list_value(args, 5)) != FL_VALUE_TYPE_FLOAT ||
            fl_value_get_type(fl_value_get_list_value(args, 6)) != FL_VALUE_TYPE_FLOAT ||
            fl_value_get_type(fl_value_get_list_value(args, 7)) != FL_VALUE_TYPE_INT ||
            fl_value_get_type(fl_value_get_list_value(args, 8)) != FL_VALUE_TYPE_INT ||
            fl_value_get_type(fl_value_get_list_value(args, 9)) != FL_VALUE_TYPE_FLOAT ||
            fl_value_get_type(fl_value_get_list_value(args, 10)) != FL_VALUE_TYPE_FLOAT ||
            fl_value_get_type(fl_value_get_list_value(args, 11)) != FL_VALUE_TYPE_INT)
        {
            fl_method_call_respond_error(method_call, "Bad Arguments", "", nullptr, nullptr);
            return;
        }
        auto const parent_id{arg<FL_VALUE_TYPE_INT, int>(args, 0)};
        MirWindowSize const size{
            .width = arg<FL_VALUE_TYPE_FLOAT, int>(args, 1),
            .height = arg<FL_VALUE_TYPE_FLOAT, int>(args, 2)};

        // Convert from anchor (originally a FlutterViewPositionerAnchor) to mir_positioner_v1_gravity
        auto const gravity{
            [](mir_positioner_v1_anchor anchor) -> mir_positioner_v1_gravity
            {
                switch (anchor)
                {
	            case MIR_POSITIONER_V1_ANCHOR_NONE: return MIR_POSITIONER_V1_GRAVITY_NONE;
                case MIR_POSITIONER_V1_ANCHOR_TOP: return MIR_POSITIONER_V1_GRAVITY_BOTTOM;
                case MIR_POSITIONER_V1_ANCHOR_BOTTOM: return MIR_POSITIONER_V1_GRAVITY_TOP;
                case MIR_POSITIONER_V1_ANCHOR_LEFT: return MIR_POSITIONER_V1_GRAVITY_RIGHT;
                case MIR_POSITIONER_V1_ANCHOR_RIGHT: return MIR_POSITIONER_V1_GRAVITY_LEFT;
                case MIR_POSITIONER_V1_ANCHOR_TOP_LEFT: return MIR_POSITIONER_V1_GRAVITY_BOTTOM_RIGHT;
                case MIR_POSITIONER_V1_ANCHOR_BOTTOM_LEFT: return MIR_POSITIONER_V1_GRAVITY_TOP_RIGHT;
                case MIR_POSITIONER_V1_ANCHOR_TOP_RIGHT: return MIR_POSITIONER_V1_GRAVITY_BOTTOM_LEFT;
                case MIR_POSITIONER_V1_ANCHOR_BOTTOM_RIGHT: return MIR_POSITIONER_V1_GRAVITY_TOP_LEFT;
                }
            }(arg<FL_VALUE_TYPE_INT, mir_positioner_v1_anchor>(args, 8))};

        MirWindowPositioner const positioner{
            .anchor_rect = {
                .x = arg<FL_VALUE_TYPE_FLOAT, int>(args, 3),
                .y = arg<FL_VALUE_TYPE_FLOAT, int>(args, 4),
                .width = arg<FL_VALUE_TYPE_FLOAT, int>(args, 5),
                .height = arg<FL_VALUE_TYPE_FLOAT, int>(args, 6)},
            .anchor = arg<FL_VALUE_TYPE_INT, mir_positioner_v1_anchor>(args, 7),
            .gravity = gravity,
            .offset = {
                .dx = arg<FL_VALUE_TYPE_FLOAT, int>(args, 9),
                .dy = arg<FL_VALUE_TYPE_FLOAT, int>(args, 10)},
            .constraint_adjustment = arg<FL_VALUE_TYPE_INT, uint32_t>(args, 11)
        };

        if (!self->windows.contains(parent_id))
        {
            fl_method_call_respond_error(method_call, "Bad Arguments", "", nullptr, nullptr);
            return;
        }

        MirWindow* const parent_mir_window{self->windows[parent_id]};

        auto const new_id{get_new_window_id(self->windows)};
        auto const archetype{[&name]()
            {
                if (name == "createSatelliteWindow")
                    return MirWindowArchetype::satellite;
                if (name == "createPopupWindow")
                    return MirWindowArchetype::popup;
                if (name == "createTipWindow")
                    return MirWindowArchetype::tip;
                return MirWindowArchetype::satellite;
            }()};
        MirWindow* const mir_window{mir_window_new(archetype, size, positioner, parent_mir_window, new_id)};
        self->windows[mir_window->id] = mir_window;
        gtk_window_set_application(GTK_WINDOW(mir_window), GTK_APPLICATION(self));
        gtk_widget_show(GTK_WIDGET(mir_window));

        g_autoptr(FlValue) result{fl_value_new_int(mir_window->id)};
        fl_method_call_respond_success(method_call, result, nullptr);
    }
    else if (name == "createDialogWindow")
    {
        FlValue* const args{fl_method_call_get_args(method_call)};
        if (fl_value_get_type(args) != FL_VALUE_TYPE_LIST || fl_value_get_length(args) != 3 ||
            fl_value_get_type(fl_value_get_list_value(args, 0)) != FL_VALUE_TYPE_FLOAT ||
            fl_value_get_type(fl_value_get_list_value(args, 1)) != FL_VALUE_TYPE_FLOAT ||
            fl_value_get_type(fl_value_get_list_value(args, 2)) != FL_VALUE_TYPE_INT)
        {
            fl_method_call_respond_error(method_call, "Bad Arguments", "", nullptr, nullptr);
            return;
        }
        MirWindowSize const size{
            .width = arg<FL_VALUE_TYPE_FLOAT, int>(args, 0),
            .height = arg<FL_VALUE_TYPE_FLOAT, int>(args, 1)};

        auto const parent_id{arg<FL_VALUE_TYPE_INT, int>(args, 2)};
        if (parent_id >= 0 && !self->windows.contains(parent_id))
        {
            fl_method_call_respond_error(method_call, "Bad Arguments", "", nullptr, nullptr);
            return;
        }
        MirWindow* const parent_mir_window{parent_id >= 0 ? self->windows[parent_id] : nullptr};

        auto const new_id{get_new_window_id(self->windows)};
        MirWindow* const mir_window{mir_window_new(MirWindowArchetype::dialog, size, {}, parent_mir_window, new_id)};
        self->windows[mir_window->id] = mir_window;
        gtk_window_set_application(GTK_WINDOW(mir_window), GTK_APPLICATION(self));
        gtk_widget_show(GTK_WIDGET(mir_window));

        g_autoptr(FlValue) result{fl_value_new_int(mir_window->id)};
        fl_method_call_respond_success(method_call, result, nullptr);
    }
    else if (name == "closeWindow")
    {
        FlValue* const args{fl_method_call_get_args(method_call)};
        if (fl_value_get_type(args) != FL_VALUE_TYPE_LIST || fl_value_get_length(args) != 1 ||
            fl_value_get_type(fl_value_get_list_value(args, 0)) != FL_VALUE_TYPE_INT)
        {
            fl_method_call_respond_error(method_call, "Bad Arguments", "", nullptr, nullptr);
            return;
        }

        auto const window_id{arg<FL_VALUE_TYPE_INT, int>(args, 0)};
        if (!self->windows.contains(window_id))
        {
            fl_method_call_respond_error(method_call, "Bad Arguments", "", nullptr, nullptr);
            return;
        }

        MirWindow* const mir_window{self->windows[window_id]};
        mfa::Globals::instance().close_window(mir_window->surface);
    }
    else if (name == "getWindowType")
    {
        FlValue* const args{fl_method_call_get_args(method_call)};
        if (fl_value_get_type(args) != FL_VALUE_TYPE_LIST || fl_value_get_length(args) != 1 ||
            fl_value_get_type(fl_value_get_list_value(args, 0)) != FL_VALUE_TYPE_INT)
        {
            fl_method_call_respond_error(method_call, "Bad Arguments", "", nullptr, nullptr);
            return;
        }

        auto window_id{arg<FL_VALUE_TYPE_INT, int>(args, 0)};
        if (!self->windows.contains(window_id))
        {
            fl_method_call_respond_error(method_call, "Bad Arguments", "", nullptr, nullptr);
            return;
        }

        MirWindow* const mir_window{self->windows[window_id]};

        g_autoptr(FlValue) result{[](MirWindowArchetype archetype)
            {
                switch(archetype)
                {
                    case MirWindowArchetype::regular:          return fl_value_new_string("regular");
                    case MirWindowArchetype::floating_regular: return fl_value_new_string("floating_regular");
                    case MirWindowArchetype::dialog:           return fl_value_new_string("dialog");
                    case MirWindowArchetype::satellite:        return fl_value_new_string("satellite");
                    case MirWindowArchetype::popup:            return fl_value_new_string("popup");
                    case MirWindowArchetype::tip:              return fl_value_new_string("tip");
                }
            }(mir_window->archetype)};
        fl_method_call_respond_success(method_call, result, nullptr);
    }
    else if (name == "getWindowSize")
    {
        FlValue* const args{fl_method_call_get_args(method_call)};
        if (fl_value_get_type(args) != FL_VALUE_TYPE_LIST || fl_value_get_length(args) != 1 ||
            fl_value_get_type(fl_value_get_list_value(args, 0)) != FL_VALUE_TYPE_INT)
        {
            fl_method_call_respond_error(method_call, "Bad Arguments", "", nullptr, nullptr);
            return;
        }

        auto const window_id{arg<FL_VALUE_TYPE_INT, int>(args, 0)};
        if (!self->windows.contains(window_id))
        {
            fl_method_call_respond_error(method_call, "Bad Arguments", "", nullptr, nullptr);
            return;
        }

        MirWindow* const mir_window{self->windows[window_id]};

        auto const width{std::holds_alternative<std::unique_ptr<mfa::XdgToplevelWindow>>(mir_window->window) ?
            std::get<std::unique_ptr<mfa::XdgToplevelWindow>>(mir_window->window)->width() :
            std::get<std::unique_ptr<mfa::XdgPopupWindow>>(mir_window->window)->width()};
        auto const height{std::holds_alternative<std::unique_ptr<mfa::XdgToplevelWindow>>(mir_window->window) ?
            std::get<std::unique_ptr<mfa::XdgToplevelWindow>>(mir_window->window)->height() :
            std::get<std::unique_ptr<mfa::XdgPopupWindow>>(mir_window->window)->height()};

        g_autoptr(FlValue) result{fl_value_new_map()};
        fl_value_set(result, fl_value_new_string("width"), fl_value_new_float(width));
        fl_value_set(result, fl_value_new_string("height"), fl_value_new_float(height));
        fl_method_call_respond_success(method_call, result, nullptr);
    }
    else
    {
        fl_method_call_respond_not_implemented(method_call, nullptr);
    }
}

// Implements GApplication::activate.
static void my_application_activate(GApplication* application)
{
    MyApplication* const self{MY_APPLICATION(application)};
    self->main_window = GTK_WINDOW(gtk_application_window_new(GTK_APPLICATION(application)));
    gtk_window_set_title(self->main_window, "mir_flutter_app");

    self->windows = {};

    gtk_window_set_default_size(self->main_window, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
    gtk_widget_show(GTK_WIDGET(self->main_window));

    GdkWindow* const gdk_window{gtk_widget_get_window(GTK_WIDGET(self->main_window))};
    GdkDisplay* const gdk_display{gdk_window_get_display(gdk_window)};
    if (!GDK_IS_WAYLAND_DISPLAY(gdk_display))
    {
        std::cerr << "This application requires a Wayland display.\n";
        std::abort();
    }
    wl_display* const display{gdk_wayland_display_get_wl_display(gdk_display)};
    if (!display)
    {
        std::cerr << "Failed to bind to wl_display.\n";
        std::abort();
    }

    mfa::Globals::instance().bind_interfaces(display);

    g_autoptr(FlDartProject) project{fl_dart_project_new()};
    fl_dart_project_set_dart_entrypoint_arguments(project, self->dart_entrypoint_arguments);

    FlView* const view{fl_view_new(project)};
    gtk_widget_show(GTK_WIDGET(view));
    gtk_container_add(GTK_CONTAINER(self->main_window), GTK_WIDGET(view));

    FlBinaryMessenger* const messenger{fl_engine_get_binary_messenger(fl_view_get_engine(view))};
    g_autoptr(FlStandardMethodCodec) codec{fl_standard_method_codec_new()};
    self->mir_window_channel = fl_method_channel_new(messenger, CHANNEL, FL_METHOD_CODEC(codec));
    fl_method_channel_set_method_call_handler(self->mir_window_channel, mir_window_method_cb, self, nullptr);

    fl_register_plugins(FL_PLUGIN_REGISTRY(view));

    gtk_widget_grab_focus(GTK_WIDGET(view));
}

// Implements GApplication::local_command_line.
static gboolean my_application_local_command_line(GApplication* application, gchar*** arguments, int* exit_status)
{
    MyApplication* const self{MY_APPLICATION(application)};
    // Strip out the first argument as it is the binary name.
    self->dart_entrypoint_arguments = g_strdupv(*arguments + 1);

    g_autoptr(GError) error{nullptr};
    if (!g_application_register(application, nullptr, &error))
    {
        g_warning("Failed to register: %s", error->message);
        *exit_status = 1;
        return TRUE;
    }

    g_application_activate(application);
    *exit_status = 0;

    return TRUE;
}

// Implements GObject::dispose.
static void my_application_dispose(GObject* object)
{
    MyApplication* const self{MY_APPLICATION(object)};
    g_clear_pointer(&self->dart_entrypoint_arguments, g_strfreev);
    g_clear_object(&self->mir_window_channel);
    G_OBJECT_CLASS(my_application_parent_class)->dispose(object);
}

static void my_application_class_init(MyApplicationClass* klass)
{
    G_APPLICATION_CLASS(klass)->activate = my_application_activate;
    G_APPLICATION_CLASS(klass)->local_command_line = my_application_local_command_line;
    G_OBJECT_CLASS(klass)->dispose = my_application_dispose;
}

static void my_application_init(MyApplication* self)
{
}

MyApplication* my_application_new()
{
    return MY_APPLICATION(g_object_new(
        my_application_get_type(),
        "application-id",
        APPLICATION_ID,
        "flags",
        G_APPLICATION_NON_UNIQUE,
        nullptr));
}
