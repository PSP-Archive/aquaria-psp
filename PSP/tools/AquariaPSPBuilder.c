/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * tools/AquariaPSPBuilder.c: GUI-based program to build a PSP game
 * directory from the commercial data distribution and a compiled-in
 * PSP executable file.
 */

#include "../src/common.h"
#include "../src/resource/package-pkg.h"
#include "../src/texture.h"

#include "quantize.h"
#include "zoom.h"

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#define OV_EXCLUDE_STATIC_CALLBACKS
#include <vorbis/vorbisfile.h>

#include <lame/lame.h>

#define PNG_USER_MEM_SUPPORTED
#include <zlib.h>
#include <png.h>
static jmp_buf png_jmpbuf;

/*************************************************************************/

/* Version number of this program (arbitrary).  This is combined with the
 * current Mercurial revision, which should be defined on the compilation
 * command line with e.g. -DHG_REVISION=\"revision\" (for GCC -- note the
 * backslashes!), to form the final version string. */
#define VERSION  "1.6"

#ifndef HG_REVISION
# error Define HG_REVISION on the compilation command line.
#endif

#define VERSION_STRING  VERSION " (r" HG_REVISION ")"

/* Data version, used to warn users if they need to regenerate the PSP data. */
#define DATA_VERSION  5

/* File into which the data version is written. */
#define DATA_VERSION_FILE  "data-version.txt"

/*************************************************************************/

/* Pregenerated/preloaded data (built by the Makefile). */

#include "AquariaPSPBuilder-eboot.i"
#include "AquariaPSPBuilder-scripts.i"
#include "AquariaPSPBuilder-ui.i"

/*-----------------------------------------------------------------------*/

/* User-selected directory paths. */
static char *path_pcdata = NULL;
static char *path_pspin = NULL;
static char *path_pspout = NULL;
static char *path_gameout = NULL;

/* Flag: Are we currently building? */
static int in_build = 0;

/* Flag: Is the build paused? */
static int paused = 0;

/* Current data filename for showing in the build frame. */
static const char *current_file = NULL;

/*-----------------------------------------------------------------------*/

/* Local function declarations. */

static int ui_init(void);
static GtkWidget *ui_get_widget(const char *name);
static int ui_show_message(const char *name, GtkWidget *parent);
static void ui_show_error(const char *format, ...);
static void ui_oom(void);

static void ui_init_connect_callback(
    GtkBuilder *builder, GObject *object, const gchar *signal_name,
    const gchar *handler_name, GObject *connect_object,
    GConnectFlags flags, gpointer user_data);

static gboolean uicb_main_window_delete(GtkWidget *widget);
static void uicb_radio_toggled(GtkWidget *widget);
static void uicb_button_pcdata(void);
static void uicb_button_pspin(void);
static void uicb_button_pspout(void);
static void uicb_button_gameout(void);
static void uicb_button_build_gendata(void);
static void uicb_button_build_reuse(void);
static void uicb_button_pause(GtkWidget *widget);
static void uicb_button_abort(void);
static void uicb_toggle_hide_filenames(void);

static void generate_data(const char *in_path, const char *out_path,
                          double progress_min, double progress_max);
static void create_icon0(const char *in_path, const char *out_path);
static void build_eboot(const char *in_path, const char *out_path);
static void build_package(const char *in_path, const char *out_path,
                          double progress_min, double progress_max);

static Texture *parse_png(const uint8_t *data, uint32_t size);
static int create_png(Texture *texture, void **data_ret, uint32_t *size_ret);

typedef void ScanDirCallback(void *filelist_ptr, uint32_t *nfiles_ptr,
                             const char *fullpath, const char *localpath,
                             uint32_t filesize, void *userdata);
static void build_scan_directory(void *filelist_ptr, uint32_t *nfiles_ptr,
                                 const char *basepath, const char *subdir,
                                 ScanDirCallback *callback, void *userdata);
static void build_read_file(const char *directory, const char *filename,
                            void **contents_ret, uint32_t *length_ret);
static void build_write_file(const char *directory, const char *filename,
                             const void *contents, int32_t length);
static void build_report_error(const char *path, int is_write,
                               const GError *error);

static void show_current_filename(void);

/*************************************************************************/
/************************** Program entry point **************************/
/*************************************************************************/

/**
 * main:  Program entry point.  Initializes the GTK+ user interface and
 * starts the main loop.
 *
 * [Parameters]
 *     argc: Command line argument count
 *     argv: Command line argument vector
 * [Return value]
 *     Zero on successful completion, nonzero if an error occurred
 */
int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);

    if (argc != 1) {
        fprintf(stderr, "Usage: %s\n", argv[0]);
        return 1;
    }

    if (!ui_init()) {
        return 1;
    }

    gtk_main();

    return 0;
}

/*************************************************************************/
/*********************** User interface management ***********************/
/*************************************************************************/

/* User interface object for use with the GtkBuilder interface. */
static GtkBuilder *ui;

/*************************************************************************/

/**
 * ui_init:  Set up the GTK+ user interface.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Nonzero on success, zero on failure
 */
static int ui_init(void)
{
    /* Create the interface. */
    ui = gtk_builder_new();
    if (!ui) {
        ui_show_error("Unable to create user interface!");
        return 0;
    }
    GError *error = NULL;
    if (gtk_builder_add_from_string(ui, ui_glade, -1, &error) == 0) {
        ui_show_error("Unable to set up user interface!\n(GTK error: %s)",
                      error->message);
        return 0;
    }
    gtk_builder_connect_signals_full(ui, ui_init_connect_callback, NULL);

    /* Set some defaults. */
    gtk_button_clicked(GTK_BUTTON(ui_get_widget("radio_about")));
    gtk_label_set_text(GTK_LABEL(ui_get_widget("label_version")),
                       VERSION_STRING);
    gtk_widget_set_sensitive(ui_get_widget("button_build_gendata"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("button_build_reuse"), FALSE);

    /* Display the main program window. */
    gtk_widget_show(ui_get_widget("main_window"));

    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * ui_get_widget:  Return a pointer to the named widget in the user
 * interface.  A shortcut for "gtk_builder_get_object(ui,name)".
 *
 * [Parameters]
 *     name: Widget name
 * [Return value]
 *     Pointer to widget, or NULL if not found
 */
static GtkWidget *ui_get_widget(const char *name)
{
    PRECOND_SOFT(ui != NULL, return NULL);
    PRECOND_SOFT(name != NULL, return NULL);
    return GTK_WIDGET(gtk_builder_get_object(ui, name));
}

/*-----------------------------------------------------------------------*/

/**
 * ui_show_message:  Display a predefined message dialog and wait for the
 * user's response.
 *
 * [Parameters]
 *       name: Message dialog widget name
 *     parent: Parent window widget
 * [Return value]
 *     Nonzero if the user clocked "OK", zero otherwise
 */
static int ui_show_message(const char *name, GtkWidget *parent)
{
    GtkWidget *message = ui_get_widget(name);
    gtk_window_set_transient_for(GTK_WINDOW(message), GTK_WINDOW(parent));
    const int response = gtk_dialog_run(GTK_DIALOG(message));
    gtk_widget_hide(message);
    return response == GTK_RESPONSE_OK;
}

/*-----------------------------------------------------------------------*/

/**
 * ui_show_error:  Display an error dialog and wait for the user to
 * dismiss it.
 *
 * [Parameters]
 *     format: Error message format string (printf()-style)
 *        ...: Format arguments
 * [Return value]
 *     None
 */
static void ui_show_error(const char *format, ...)
{
    char buf[10000];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(ui_get_widget("main_window")),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        "%s", buf
    );
    gtk_window_set_title(GTK_WINDOW(dialog), "Error");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/*-----------------------------------------------------------------------*/

/**
 * ui_oom:  Show an out-of-memory error message (if possible) and exit the
 * program with an unsuccessful exit code.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Does not return
 */
static void ui_oom(void)
{
    ui_show_error("Internal error: out of memory! Exiting.");
    exit(1);
}

/*************************************************************************/

/**
 * ui_init_connect_callback:  Callback function for connecting signals to
 * user interface widgets, using a table of signal handler functions.
 * While this requires more maintenance than the GModule introspection
 * method used by gtk_builder_connect_signals(), it works across multiple
 * platforms without the need for special function attributes or linker
 * flags.
 *
 * [Parmaeters]
 *            builder: GtkBuilder object (unused)
 *             object: Object on which the signal handler is to be installed
 *        signal_name: Name of the signal to connect to
 *       handler_name: Signal handler function name
 *     connect_object: Related object for g_signal_connect_object()
 *              flags: Flags for g_signal_connect()
 *          user_data: User data for signal
 * [Return value]
 *     None
 */
static void ui_init_connect_callback(
    GtkBuilder *builder, GObject *object, const gchar *signal_name,
    const gchar *handler_name, GObject *connect_object,
    GConnectFlags flags, gpointer user_data)
{
    static const struct {const char *name; void *funcptr;} funcs[] = {
        {"gtk_main_quit",               gtk_main_quit},
        {"uicb_main_window_delete",     uicb_main_window_delete},
        {"uicb_radio_toggled",          uicb_radio_toggled},
        {"uicb_button_pcdata",          uicb_button_pcdata},
        {"uicb_button_pspin",           uicb_button_pspin},
        {"uicb_button_pspout",          uicb_button_pspout},
        {"uicb_button_gameout",         uicb_button_gameout},
        {"uicb_button_build_gendata",   uicb_button_build_gendata},
        {"uicb_button_build_reuse",     uicb_button_build_reuse},
        {"uicb_button_pause",           uicb_button_pause},
        {"uicb_button_abort",           uicb_button_abort},
        {"uicb_toggle_hide_filenames",  uicb_toggle_hide_filenames},
    };

    unsigned int i;
    for (i = 0; i < lenof(funcs); i++) {
        if (strcmp(funcs[i].name, handler_name) == 0) {
            if (connect_object) {
                g_signal_connect_object(object, signal_name, funcs[i].funcptr,
                                        connect_object, flags);
            } else {
                g_signal_connect_data(object, signal_name, funcs[i].funcptr,
                                      user_data, NULL, flags);
            }
            return;
        }
    }

    DMSG("WARNING: Handler %s for signal %s not found!",
         handler_name, signal_name);
}

/*************************************************************************/
/*********************** User interface callbacks ************************/
/*************************************************************************/

/**
 * uicb_main_window_delete:  Callback for delete events on the main window.
 * If a build is currently running, this displays a dialog asking the user
 * to confirm quitting before actually aborting the build.
 */
static gboolean uicb_main_window_delete(GtkWidget *widget)
{
    if (in_build) {
        return ui_show_message("message_quit_check",
                               ui_get_widget("main_window")) ? FALSE : TRUE;
    } else {
        return FALSE;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * uicb_radio_toggled:  Callback for changes in the selected menu radio
 * button.  Switches the displayed user interface page depending on the
 * selected button.
 */
static void uicb_radio_toggled(GtkWidget *widget)
{
    /* Don't react on deselects. */
    if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
        return;
    }

    if (widget == ui_get_widget("radio_gendata")) {
        gtk_widget_hide(ui_get_widget("frame_reuse"));
        gtk_widget_hide(ui_get_widget("frame_about"));
        gtk_widget_show(ui_get_widget("frame_gendata"));
    } else if (widget == ui_get_widget("radio_reuse")) {
        gtk_widget_hide(ui_get_widget("frame_gendata"));
        gtk_widget_hide(ui_get_widget("frame_about"));
        gtk_widget_show(ui_get_widget("frame_reuse"));
    } else if (widget == ui_get_widget("radio_about")) {
        gtk_widget_hide(ui_get_widget("frame_gendata"));
        gtk_widget_hide(ui_get_widget("frame_reuse"));
        gtk_widget_show(ui_get_widget("frame_about"));
    } else {
        DMSG("BOGUS: got radio_toggled callback for invalid widget %p", widget);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * uicb_button_pcdata:  Callback for the "select PC data directory" button.
 */
static void uicb_button_pcdata(void)
{
    GtkWidget *dialog = ui_get_widget("dirchooser_pcdata");
    if (path_pcdata) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), path_pcdata);
    }

    while (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        char buf[10000];
        if (strlen(path+8) >= sizeof(buf)) {
            ui_show_error("Internal error: pathname too long!\n"
                          "Try another folder.");
            continue;
        }

        if ((snprintf(buf,sizeof(buf),"%s/data",   path), g_access(buf,0) != 0)
         || (snprintf(buf,sizeof(buf),"%s/gfx",    path), g_access(buf,0) != 0)
         || (snprintf(buf,sizeof(buf),"%s/mus",    path), g_access(buf,0) != 0)
         || (snprintf(buf,sizeof(buf),"%s/scripts",path), g_access(buf,0) != 0)
         || (snprintf(buf,sizeof(buf),"%s/sfx",    path), g_access(buf,0) != 0)
         || (snprintf(buf,sizeof(buf),"%s/vox",    path), g_access(buf,0) != 0)
         || (snprintf(buf,sizeof(buf),"%s/gfx/title/logo.png",path),
                                                          g_access(buf,0) != 0)
        ) {
            ui_show_message("message_bad_sourcedir", dialog);
            g_free(path);
            continue;
        }

        free(path_pcdata);
        path_pcdata = strdup(path);
        g_free(path);
        if (!path_pcdata) {
            ui_oom();
        }
        gtk_label_set_text(GTK_LABEL(ui_get_widget("label_gendata_pcdata")),
                           path_pcdata);
        gtk_widget_set_sensitive(ui_get_widget("button_build_gendata"),
                                 path_pcdata && path_pspout && path_gameout);
        break;
    }

    gtk_widget_hide(dialog);
}

/*-----------------------------------------------------------------------*/

/**
 * uicb_button_pspin:  Callback for the "select PSP data directory" button.
 */
static void uicb_button_pspin(void)
{
    GtkWidget *dialog = ui_get_widget("dirchooser_pspin");
    if (path_pspin) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), path_pspin);
    }

    while (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        char buf[10000];
        if (strlen(path+8) >= sizeof(buf)) {
            ui_show_error("Internal error: pathname too long!\n"
                          "Try another folder.");
            continue;
        }

        if ((snprintf(buf,sizeof(buf),"%s/data",   path), g_access(buf,0) != 0)
         || (snprintf(buf,sizeof(buf),"%s/gfx",    path), g_access(buf,0) != 0)
         || (snprintf(buf,sizeof(buf),"%s/mus",    path), g_access(buf,0) != 0)
         || (snprintf(buf,sizeof(buf),"%s/scripts",path), g_access(buf,0) != 0)
         || (snprintf(buf,sizeof(buf),"%s/sfx",    path), g_access(buf,0) != 0)
         || (snprintf(buf,sizeof(buf),"%s/vox",    path), g_access(buf,0) != 0)
        ) {
            ui_show_message("message_bad_sourcedir", dialog);
            g_free(path);
            continue;
        }

        int ok = 0;
        snprintf(buf, sizeof(buf), "%s/%s", path, DATA_VERSION_FILE);
        FILE *f = fopen(buf, "r");
        if (f) {
            int version = 0;
            ok = (fscanf(f, "%i", &version) == 1 && version == DATA_VERSION);
            fclose(f);
        }
        if (!ok) {
            ui_show_message("message_psp_out_of_date", dialog);
            g_free(path);
            continue;
        }

        free(path_pspin);
        path_pspin = strdup(path);
        g_free(path);
        if (!path_pspin) {
            ui_oom();
        }
        gtk_label_set_text(GTK_LABEL(ui_get_widget("label_reuse_pspin")),
                           path_pspin);
        gtk_widget_set_sensitive(ui_get_widget("button_build_reuse"),
                                 path_pspin && path_gameout);
        break;
    }

    gtk_widget_hide(dialog);
}

/*-----------------------------------------------------------------------*/

/**
 * uicb_button_pspout:  Callback for the "select new PSP data directory"
 * button.
 */
static void uicb_button_pspout(void)
{
    GtkWidget *dialog = ui_get_widget("dirchooser_pspout");
    if (path_pspout) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), path_pspout);
    }

    while (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (!path) {
            ui_show_error("Internal error: no filename returned!\n"
                          "Make sure \"Recently Used\" is not selected.");
            continue;
        }
        char buf[10000];
        if (strlen(path+8) >= sizeof(buf)) {
            ui_show_error("Internal error: pathname too long!\n"
                          "Try another folder.");
            continue;
        }

        if ((snprintf(buf,sizeof(buf),"%s/data",   path), g_access(buf,0) == 0)
         || (snprintf(buf,sizeof(buf),"%s/gfx",    path), g_access(buf,0) == 0)
         || (snprintf(buf,sizeof(buf),"%s/mus",    path), g_access(buf,0) == 0)
         || (snprintf(buf,sizeof(buf),"%s/scripts",path), g_access(buf,0) == 0)
         || (snprintf(buf,sizeof(buf),"%s/sfx",    path), g_access(buf,0) == 0)
         || (snprintf(buf,sizeof(buf),"%s/vox",    path), g_access(buf,0) == 0)
        ) {
            if (!ui_show_message("message_overwrite_pspout", dialog)) {
                g_free(path);
                continue;
            }
        } else {
            int empty = 1;
            GDir *dir = g_dir_open(path, 0, NULL);
            if (dir) {
                empty = (g_dir_read_name(dir) == NULL);
                g_dir_close(dir);
            }
            if (!empty) {
                if (!ui_show_message("message_overwrite_generic", dialog)) {
                    g_free(path);
                    continue;
                }
            }
        }

        free(path_pspout);
        path_pspout = strdup(path);
        g_free(path);
        if (!path_pspout) {
            ui_oom();
        }
        gtk_label_set_text(GTK_LABEL(ui_get_widget("label_gendata_pspout")),
                           path_pspout);
        gtk_widget_set_sensitive(ui_get_widget("button_build_gendata"),
                                 path_pcdata && path_pspout && path_gameout);
        break;
    }

    gtk_widget_hide(dialog);
}

/*-----------------------------------------------------------------------*/

/**
 * uicb_button_gameout:  Callback for the "select output directory" button.
 */
static void uicb_button_gameout(void)
{
    GtkWidget *dialog = ui_get_widget("dirchooser_gameout");
    if (path_gameout) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), path_gameout);
    }

    while (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (!path) {
            ui_show_error("Internal error: no filename returned!\n"
                          "Make sure \"Recently Used\" is not selected.");
            continue;
        }
        char buf[10000];
        if (strlen(path+8) >= sizeof(buf)) {
            ui_show_error("Internal error: pathname too long!\n"
                          "Try another folder.");
            continue;
        }

        if ((snprintf(buf,sizeof(buf),"%s/aquaria.dat",path), g_access(buf,0) == 0)) {
            if (!ui_show_message("message_overwrite_gameout", dialog)) {
                g_free(path);
                continue;
            }
        } else {
            int empty = 1;
            GDir *dir = g_dir_open(path, 0, NULL);
            if (dir) {
                empty = (g_dir_read_name(dir) == NULL);
                g_dir_close(dir);
            }
            if (!empty) {
                if (!ui_show_message("message_overwrite_generic", dialog)) {
                    g_free(path);
                    continue;
                }
            }
        }

        free(path_gameout);
        path_gameout = strdup(path);
        g_free(path);
        if (!path_gameout) {
            ui_oom();
        }
        gtk_label_set_text(GTK_LABEL(ui_get_widget("label_gendata_gameout")),
                           path_gameout);
        gtk_label_set_text(GTK_LABEL(ui_get_widget("label_reuse_gameout")),
                           path_gameout);
        gtk_widget_set_sensitive(ui_get_widget("button_build_gendata"),
                                 path_pcdata && path_gameout && path_gameout);
        gtk_widget_set_sensitive(ui_get_widget("button_build_reuse"),
                                 path_pspin && path_gameout);
        break;
    }

    gtk_widget_hide(dialog);
}

/*-----------------------------------------------------------------------*/

/**
 * uicb_button_build_gendata:  Callback for the "start build" button for
 * generating a new PSP data directory.
 */
static void uicb_button_build_gendata(void)
{
    if (strcmp(path_pcdata, path_pspout) == 0) {
        ui_show_message("message_need_separate_dirs",
                        ui_get_widget("main_window"));
        return;
    }

    gtk_widget_set_sensitive(ui_get_widget("label_menu_title"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("radio_gendata"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("label_gendata"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("radio_reuse"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("label_reuse"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("radio_about"), FALSE);
    gtk_widget_hide(ui_get_widget("frame_gendata"));
    gtk_widget_hide(ui_get_widget("frame_reuse"));
    gtk_widget_hide(ui_get_widget("frame_about"));
    gtk_widget_show(ui_get_widget("frame_build"));
    gtk_widget_set_sensitive(ui_get_widget("label_build_file_title"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("label_build_file"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("label_build_count_title"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("label_build_count"), FALSE);

    in_build = 1;
    create_icon0(path_pcdata, path_pspout);
    generate_data(path_pcdata, path_pspout, 0.0, 0.985);
    build_eboot(path_pspout, path_gameout);
    build_package(path_pspout, path_gameout, 0.99, 1.0);
    in_build = 0;

    gtk_label_set_text(GTK_LABEL(ui_get_widget("label_build_status")),
                       "Finished!");
    gtk_label_set_text(GTK_LABEL(ui_get_widget("label_build_file")),
                       "(none)");
    gtk_widget_set_sensitive(ui_get_widget("label_build_file_title"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("label_build_file"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("label_build_count_title"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("label_build_count"), FALSE);
    gtk_progress_bar_set_fraction(
        GTK_PROGRESS_BAR(ui_get_widget("progress_build")), 1.0
    );
    gtk_progress_bar_set_text(
        GTK_PROGRESS_BAR(ui_get_widget("progress_build")), "Finished!"
    );
    gtk_widget_set_sensitive(ui_get_widget("check_hide_filenames"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("label_hide_filenames"), FALSE);
    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(ui_get_widget("button_build_pause")), FALSE
    );
    gtk_widget_set_sensitive(ui_get_widget("button_build_pause"), FALSE);
    gtk_widget_hide(ui_get_widget("button_build_abort"));
    gtk_widget_show(ui_get_widget("button_build_quit"));
}

/*-----------------------------------------------------------------------*/

/**
 * uicb_button_build_reuse:  Callback for the "start build" button for
 * reusing an existing PSP data directory.
 */
static void uicb_button_build_reuse(void)
{
    gtk_widget_set_sensitive(ui_get_widget("label_menu_title"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("radio_gendata"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("label_gendata"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("radio_reuse"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("label_reuse"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("radio_about"), FALSE);
    gtk_widget_hide(ui_get_widget("frame_gendata"));
    gtk_widget_hide(ui_get_widget("frame_reuse"));
    gtk_widget_hide(ui_get_widget("frame_about"));
    gtk_widget_show(ui_get_widget("frame_build"));
    gtk_widget_set_sensitive(ui_get_widget("label_build_file_title"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("label_build_file"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("label_build_count_title"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("label_build_count"), FALSE);

    in_build = 1;
    build_eboot(path_pspin, path_gameout);
    build_package(path_pspin, path_gameout, 0.0, 1.0);
    in_build = 0;

    gtk_label_set_text(GTK_LABEL(ui_get_widget("label_build_status")),
                       "Finished!");
    gtk_label_set_text(GTK_LABEL(ui_get_widget("label_build_file")),
                       "(none)");
    gtk_widget_set_sensitive(ui_get_widget("label_build_file_title"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("label_build_file"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("label_build_count_title"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("label_build_count"), FALSE);
    gtk_progress_bar_set_fraction(
        GTK_PROGRESS_BAR(ui_get_widget("progress_build")), 1.0
    );
    gtk_progress_bar_set_text(
        GTK_PROGRESS_BAR(ui_get_widget("progress_build")), "Finished!"
    );
    gtk_widget_set_sensitive(ui_get_widget("check_hide_filenames"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("label_hide_filenames"), FALSE);
    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(ui_get_widget("button_build_pause")), FALSE
    );
    gtk_widget_set_sensitive(ui_get_widget("button_build_pause"), FALSE);
    gtk_widget_hide(ui_get_widget("button_build_abort"));
    gtk_widget_show(ui_get_widget("button_build_quit"));
}

/*-----------------------------------------------------------------------*/

/**
 * uicb_button_pause:  Callback for the build frame "pause" button.
 */
static void uicb_button_pause(GtkWidget *widget)
{
    paused = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    while (paused) {
        if (gtk_main_iteration()) {
            gtk_main_quit();  // Pass the quit event up the chain.
            return;
        }
    }
}

/*-----------------------------------------------------------------------*/

/**
 * uicb_button_abort:  Callback for the build frame "abort" button.
 */
static void uicb_button_abort(void)
{
    if (ui_show_message("message_abort_check", ui_get_widget("main_window"))) {
        gtk_main_quit();
    }
}

/*-----------------------------------------------------------------------*/

/**
 * uicb_button_abort:  Callback for the build frame "hide filenames"
 * checkbox.
 */
static void uicb_toggle_hide_filenames(void)
{
    show_current_filename();
}

/*************************************************************************/
/****************** Build routines and helper functions ******************/
/*************************************************************************/

/**
 * GTK_MAIN_ITERATION_OR_EXIT:  Convenience macro to run a GTK+ main loop
 * iteration and exit the program if a gtk_main_quit() is detected.
 */
#define GTK_MAIN_ITERATION_OR_EXIT()  do {  \
    while (gtk_events_pending()) {          \
        if (gtk_main_iteration_do(FALSE)) { \
            exit(0);                        \
        }                                   \
    }                                       \
} while (0)

/**
 * SET_PROGRESS_AND_ITERATE:  Convenience macro to update the progress bar
 * to the specified fraction and call GTK_MAIN_ITERATION_OR_EXIT().
 */
#define SET_PROGRESS_AND_ITERATE(progress)  do {                        \
    int percent = (int)floor((progress) * 100);                         \
    char percentbuf[8];                                                 \
    snprintf(percentbuf, sizeof(percentbuf), "%d%%", percent);          \
    gtk_progress_bar_set_fraction(                                      \
        GTK_PROGRESS_BAR(ui_get_widget("progress_build")), (progress)   \
    );                                                                  \
    gtk_progress_bar_set_text(                                          \
        GTK_PROGRESS_BAR(ui_get_widget("progress_build")), percentbuf   \
    );                                                                  \
    GTK_MAIN_ITERATION_OR_EXIT();                                       \
} while (0)

/*************************************************************************/

/**
 * create_icon0:  Create the ICON0.PNG (menu icon) file for the game and
 * store it in the PSP data directory.
 *
 * [Parameters]
 *      in_path: Input (PC data) directory path
 *     out_path: Output (PSP data) directory path
 * [Return value]
 *     None
 * [Notes]
 *     If an unrecoverable error occurs, this routine terminates the program.
 */
static void create_icon0(const char *in_path, const char *out_path)
{
    current_file = "ICON0.PNG";
    show_current_filename();
    gtk_label_set_text(GTK_LABEL(ui_get_widget("label_build_count")), "---");
    gtk_widget_set_sensitive(ui_get_widget("label_build_count_title"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("label_build_count"), FALSE);
    GTK_MAIN_ITERATION_OR_EXIT();

    /* Read in the "Aquaria" logo used on the title screen. */

    void *pngdata;
    uint32_t pngsize;
    build_read_file(in_path, "gfx/title/logo.png", &pngdata, &pngsize);

    Texture *texture;
    texture = parse_png(pngdata, pngsize);
    if (!texture) {
        fprintf(stderr, "Failed to parse PNG file\n");
        ui_show_error("The file \"gfx/title/logo.png\" is corrupt!"
                      " Unable to continue; the build will now abort.");
        exit(1);
    }

    free(pngdata);

    /* Cut off the empty borders to get a 9:5 aspect ratio, then shrink to
     * the 144x80 icon size. */

    const int left = 210, top = 89, width = 630, height = 350;
    const uint8_t *in = &texture->pixels[(top * texture->stride + left) * 4];
    uint8_t *out = texture->pixels;
    int y;
    for (y = 0; y < height; y++, in += texture->stride*4, out += width*4) {
        memcpy(out, in, width*4);
    }
    texture->width  = width;
    texture->height = height;
    texture->stride = width;

    const int new_width  = 144;
    const int new_height = 80;
    const int new_stride = 144;
    void *tempbuf = malloc(new_stride * new_height * 4);
    if (!tempbuf) {
        fprintf(stderr, "Out of memory for shrink buffer (%d bytes)\n",
                new_width * new_height * 4);
        ui_oom();
    }

    ZoomInfo *zi = zoom_init(texture->width, texture->height,
                             new_width, new_height,
                             4, texture->stride*4, new_stride*4,
                             1, TCV_ZOOM_CUBIC_KEYS4);
    if (!zi) {
        fprintf(stderr, "zoom_init() failed\n");
       ui_oom();
    }
    zoom_process(zi, texture->pixels, tempbuf);
    zoom_free(zi);

    texture->width  = new_width;
    texture->height = new_height;
    texture->stride = new_stride;
    memcpy(texture->pixels, tempbuf, new_stride * new_height * 4);
    free(tempbuf);

    /* Write out the icon as a PNG file. */

    if (!create_png(texture, &pngdata, &pngsize)) {
        fprintf(stderr, "Failed to create PNG data for icon\n");
        ui_show_error("An error occurred while generating PNG data!"
                      " Unable to continue; the build will now abort.");
        exit(1);
    }
    build_write_file(out_path, "ICON0.PNG", pngdata, pngsize);
    free(pngdata);
}

/*************************************************************************/

typedef struct FileListEntry_ {
    char *path;
    uint32_t size;
} FileListEntry;

static int stricmp_ptr(const void *a, const void *b)
    {return stricmp(*(const char **)a, *(const char **)b);}

static void add_to_file_list(FileListEntry **filelist_ptr,
                             uint32_t *nfiles_ptr,
                             const char *fullpath, const char *localpath,
                             uint32_t filesize, void *userdata);
static int generate_tex(const char *pngpath,
                        const void *pngdata, uint32_t pngsize,
                        void **texdata_ret, uint32_t *texsize_ret,
                        double progress_min, double progress_max);
static int generate_mp3(const void *oggdata, uint32_t oggsize,
                        void **mp3data_ret, uint32_t *mp3size_ret,
                        double progress_min, double progress_max);

/*-----------------------------------------------------------------------*/

/**
 * generate_data:  Generate PSP data from the Aquaria PC data.
 *
 * [Parameters]
 *          in_path: Input (PC data) directory path
 *         out_path: Output (PSP data) directory path
 *     progress_min: Initial progress value (0.0-1.0)
 *     progress_max: Final progress value (0.0-1.0)
 * [Return value]
 *     None
 * [Notes]
 *     If an unrecoverable error occurs, this routine terminates the program.
 */
static void generate_data(const char *in_path, const char *out_path,
                          double progress_min, double progress_max)
{
    FileListEntry *filelist;
    uint32_t nfiles;
    uint32_t bytes_png, bytes_ogg, bytes_other;
    uint32_t bdone_png, bdone_ogg, bdone_other;
    uint32_t i;

    #define CALC_PROGRESS()                                             \
        (progress_min                                                   \
         + (((bdone_png*129.1 + bdone_ogg*129.0 + bdone_other*1.0)      \
             / (bytes_png*129.1 + bytes_ogg*129.0 + bytes_other*1.0))   \
            * (progress_max - progress_min)))

    gtk_label_set_text(GTK_LABEL(ui_get_widget("label_build_status")),
                       "Generating PSP data files...");
    GTK_MAIN_ITERATION_OR_EXIT();

    filelist = NULL;
    nfiles = 0;
    build_scan_directory(&filelist, &nfiles, in_path, "data",
                         (ScanDirCallback *)add_to_file_list, NULL);
    GTK_MAIN_ITERATION_OR_EXIT();
    build_scan_directory(&filelist, &nfiles, in_path, "gfx",
                         (ScanDirCallback *)add_to_file_list, NULL);
    GTK_MAIN_ITERATION_OR_EXIT();
    build_scan_directory(&filelist, &nfiles, in_path, "mus",
                         (ScanDirCallback *)add_to_file_list, NULL);
    GTK_MAIN_ITERATION_OR_EXIT();
    build_scan_directory(&filelist, &nfiles, in_path, "scripts",
                         (ScanDirCallback *)add_to_file_list, NULL);
    GTK_MAIN_ITERATION_OR_EXIT();
    build_scan_directory(&filelist, &nfiles, in_path, "sfx",
                         (ScanDirCallback *)add_to_file_list, NULL);
    GTK_MAIN_ITERATION_OR_EXIT();
    build_scan_directory(&filelist, &nfiles, in_path, "vox",
                         (ScanDirCallback *)add_to_file_list, NULL);
    GTK_MAIN_ITERATION_OR_EXIT();
    qsort(filelist, nfiles, sizeof(*filelist), stricmp_ptr);
    GTK_MAIN_ITERATION_OR_EXIT();

    bytes_png = bytes_ogg = bytes_other = 0;
    for (i = 0; i < nfiles; i++) {
        const char *path = filelist[i].path;
        if (strlen(path) < 4) {  // Should be impossible, but just in case
            bytes_other += filelist[i].size;
        } else if (stricmp(path + strlen(path)-4, ".png") == 0) {
            bytes_png += filelist[i].size;
        } else if (stricmp(path + strlen(path)-4, ".ogg") == 0) {
            bytes_ogg += filelist[i].size;
        } else if (stricmp(path + strlen(path)-4, ".lua") == 0) {
            // Ignore
        } else {
            bytes_other += filelist[i].size;
        }
    }

    gtk_widget_set_sensitive(ui_get_widget("label_build_count_title"), TRUE);
    gtk_widget_set_sensitive(ui_get_widget("label_build_count"), TRUE);

    bdone_png = bdone_ogg = bdone_other = 0;
    for (i = 0; i < nfiles; i++) {
        current_file = filelist[i].path;
        show_current_filename();
        char countbuf[32];
        snprintf(countbuf, sizeof(countbuf), "%u/%u", i+1, nfiles);
        gtk_label_set_text(GTK_LABEL(ui_get_widget("label_build_count")),
                           countbuf);
        const double progress = CALC_PROGRESS();
        SET_PROGRESS_AND_ITERATE(progress);

        const char *path = filelist[i].path;
        void *filedata;
        uint32_t filesize;
        build_read_file(in_path, path, &filedata, &filesize);

        if (strlen(path) >= 4
         && stricmp(path + strlen(path)-4, ".png") == 0
        ) {
            /* Convert .png to .tex */
            bdone_png += filelist[i].size;
            const double next_progress = CALC_PROGRESS();

            void *texdata = NULL;
            uint32_t texsize = 0;
            while (!generate_tex(path, filedata, filesize, &texdata, &texsize,
                                 progress, next_progress)) {
                free(texdata);
                build_report_error(path, 0,
                                   &((GError){.message = "Failed to convert PNG image to PSP texture"}));
            }
            /* Write out a dummy .png file so BBGE can find the texture. */
            build_write_file(out_path, path, "", 0);
            char *texpath = strdup(path);
            if (!texpath) {
                ui_oom();
            }
            memcpy(texpath + strlen(texpath)-3, "tex", 3);
            build_write_file(out_path, texpath, texdata, texsize);
            free(texpath);
            free(texdata);

        } else if (strlen(path) >= 4
                && stricmp(path + strlen(path)-4, ".ogg") == 0
        ) {
            /* Convert .ogg to .mp3 */
            bdone_ogg += filelist[i].size;
            const double next_progress = CALC_PROGRESS();

            void *mp3data = NULL;
            uint32_t mp3size = 0;
            while (!generate_mp3(filedata, filesize, &mp3data, &mp3size,
                                 progress, next_progress)) {
                free(mp3data);
                build_report_error(path, 0,
                                   &((GError){.message = "Failed to convert Ogg audio to MP3"}));
            }
            /* Write out a dummy .ogg file so BBGE can find the sound. */
            build_write_file(out_path, path, "", 0);
            char *mp3path = strdup(path);
            if (!mp3path) {
                ui_oom();
            }
            memcpy(mp3path + strlen(mp3path)-3, "mp3", 3);
            build_write_file(out_path, mp3path, mp3data, mp3size);
            free(mp3path);
            free(mp3data);

        } else if (strlen(path) >= 4
                && stricmp(path + strlen(path)-4, ".lua") == 0
        ) {
            /* Ignore .lua files entirely */

        } else {
            /* Other files are copied straight over */
            bdone_other += filelist[i].size;
            build_write_file(out_path, path, filedata, filesize);
        }

        g_free(filedata);
        free(filelist[i].path);
    }

    free(filelist);

    char buf[16];
    if (snprintf(buf, sizeof(buf), "%d", DATA_VERSION) >= sizeof(buf)) {
        ui_show_error("Internal error: buffer overflow!\n"
                      "Aborting the program.");
        exit(1);
    }
    build_write_file(out_path, DATA_VERSION_FILE, buf, strlen(buf));

    #undef CALC_PROGRESS
}

/*----------------------------------*/

/**
 * add_to_file_list:  Directory scan callback which adds files to the
 * list for PSP data generation.
 *
 * [Parameters]
 *     filelist_ptr: Pointer to the variable holding the file list array
 *       nfiles_ptr: Pointer to the variable holding the number of files
 *         fullpath: Path of file in host filesystem
 *        localpath: Corresponding directory path for package file
 *         filesize: Size of file (bytes)
 *         userdata: User data pointer (unused)
 * [Return value]
 *     None
 */
static void add_to_file_list(FileListEntry **filelist_ptr,
                             uint32_t *nfiles_ptr,
                             const char *fullpath, const char *localpath,
                             uint32_t filesize, void *userdata)
{
    (*nfiles_ptr)++;
    *filelist_ptr =
        realloc(*filelist_ptr, *nfiles_ptr * sizeof(**filelist_ptr));
    if (!*filelist_ptr) {
        ui_oom();
    }
    (*filelist_ptr)[(*nfiles_ptr)-1].path = strdup(localpath);
    (*filelist_ptr)[(*nfiles_ptr)-1].size = filesize;
    if (!(*filelist_ptr)[(*nfiles_ptr)-1].path) {
        ui_oom();
    }
}

/*-----------------------------------------------------------------------*/

/* List of texture files that get special processing.  If there's no match,
 * the default processing of shrinking to half size is applied. */

static const struct {
    const char *path;  // Local path of file to which this entry applies.
    int clip_x, clip_y, clip_w, clip_h;  // Clip region (only if w/h nonzero)
    float scale;       // Scale factor (only if nonzero)
} tex_special_list[] = {

    /* World map UI: scale to native size (272/600 * 1.4 scale factor used
     * in WorldMapRender.cpp).  The clip region is chosen to give an output
     * texture size of 512x64. */
    {"gfx/gui/worldmap-ui.png",
     .clip_x = 109, .clip_y = 0, .clip_w = 806, .clip_h = 101,
     .scale = (272.0/600.0) * 1.4
    },

};

/*----------------------------------*/

/* Periodic callback for generate_palette(). */
static void genpal_callback(void) { GTK_MAIN_ITERATION_OR_EXIT(); }

/* Other local helper functions. */
static Texture *parse_png(const uint8_t *data, uint32_t size);
static int clip_texture(Texture *tex, int left, int top, int width, int height);
static int shrink_texture(Texture *tex, int new_width, int new_height);
static int quantize_texture(Texture *tex);
static int swizzle_texture(Texture *tex);
static int generate_texfile(Texture *tex, void **data_ret, uint32_t *size_ret);

/*----------------------------------*/

/**
 * generate_tex:  Convert a PNG image to our custom PSP texture format.
 *
 * [Parameters]
 *          pngpath: Local pathname of PNG file
 *          pngdata: PNG file data buffer
 *          pngsize: PNG file data size (bytes)
 *      texdata_ret: Pointer to variable to receive the *.tex data buffer
 *      texsize_ret: Pointer to variable to receive the *.tex data size (bytes)
 *     progress_min: Initial progress value (0.0-1.0)
 *     progress_max: Final progress value (0.0-1.0)
 * [Return value]
 *     Nonzero on success, zero on failure
 */
static int generate_tex(const char *pngpath,
                        const void *pngdata, uint32_t pngsize,
                        void **texdata_ret, uint32_t *texsize_ret,
                        double progress_min, double progress_max)
{
    const double progress_delta = progress_max - progress_min;
    void *texdata = NULL;
    uint32_t texsize = 0;

    Texture *texture;

    /* First decode the PNG file. */

    texture = parse_png(pngdata, pngsize);
    if (!texture) {
        fprintf(stderr, "Failed to parse PNG file\n");
        goto error_return;
    }
    SET_PROGRESS_AND_ITERATE(progress_min + 0.05*progress_delta);

    /* See if there are any special operations to apply to this file.
     * If not, apply the default transformation of shrinking to half size. */

    unsigned int i;
    for (i = 0; i < lenof(tex_special_list); i++) {
        if (stricmp(pngpath, tex_special_list[i].path) == 0) {
            break;
        }
    }

    if (i < lenof(tex_special_list)) {

        if (tex_special_list[i].clip_w && tex_special_list[i].clip_h) {
            if (!clip_texture(texture,
                              tex_special_list[i].clip_x,
                              tex_special_list[i].clip_y,
                              tex_special_list[i].clip_w,
                              tex_special_list[i].clip_h)) {
                fprintf(stderr, "Failed to clip image\n");
                goto error_free_texture;
            }
        }

        if (tex_special_list[i].scale && tex_special_list[i].scale != 1) {
            const float scale = tex_special_list[i].scale;
            if (!shrink_texture(texture,
                                (int)roundf(texture->width * scale),
                                (int)roundf(texture->height * scale))) {
                fprintf(stderr, "Failed to shrink image\n");
                goto error_free_texture;
            }
        }

    } else {  // No special operations to be performed.

        /* Don't bother shrinking if the width or height is already 1. */
        if (texture->width != 1 && texture->height != 1) {
            if (!shrink_texture(texture, texture->width/2, texture->height/2)) {
                fprintf(stderr, "Failed to shrink image\n");
                goto error_free_texture;
            }
        }

    }

    SET_PROGRESS_AND_ITERATE(progress_min + 0.35*progress_delta);

    /* Generate a color palette for the image. */

    const uint32_t total_pixels = texture->width * texture->height;
    uint32_t palette[256];
    memset(palette, 0, sizeof(palette));
    generate_palette((uint32_t *)texture->pixels, total_pixels, 1,
                     total_pixels, palette, 0, genpal_callback);
    SET_PROGRESS_AND_ITERATE(progress_min + 0.80*progress_delta);

    /* Quantize the texture using the given palette and generate a
     * PSP texture file. */

    texture->palette = palette;

    if (!quantize_texture(texture)) {
        fprintf(stderr, "Failed to quantize image\n");
        goto error_free_texture;
    }
    SET_PROGRESS_AND_ITERATE(progress_min + 0.99*progress_delta);

    if (!swizzle_texture(texture)) {
        fprintf(stderr, "Failed to swizzle image\n");
        goto error_free_texture;
    }

    if (!generate_texfile(texture, &texdata, &texsize)) {
        fprintf(stderr, "Failed to generate texture file\n");
        goto error_free_texture;
    }

    free(texture);

    /* All done! */

    *texdata_ret = texdata;
    *texsize_ret = texsize;
    return 1;

    /* On error, clean up after ourselves. */

  error_free_texture:
    free(texture);
  error_return:
    free(texdata);
    return 0;
}

/*----------------------------------*/

/**
 * clip_texture:  Clip the given texture to the given region.  The texture
 * buffer is _not_ reallocated.
 *
 * [Parameters]
 *               tex: Texture to shrink
 *         left, top: Coordinates of upper-left corner of clip region (pixels)
 *     width, height: Size of clip region (pixels)
 * [Return value]
 *     Nonzero on success, zero on error
 */
static int clip_texture(Texture *tex, int left, int top, int width, int height)
{
    uint32_t new_stride = align_up(width, 4);
    const uint8_t *src = &tex->pixels[(top*tex->stride + left) * 4];
    uint8_t *dest = tex->pixels;
    int y;
    for (y = 0; y < height; y++, src += tex->stride*4, dest += new_stride*4) {
        memcpy(dest, src, width*4);
    }

    if (tex->empty_l > left) {
        tex->empty_l -= left;
    } else {
        tex->empty_l = 0;
    }
    if (tex->empty_t > top) {
        tex->empty_t -= top;
    } else {
        tex->empty_t = 0;
    }
    if (tex->empty_r > tex->width - (left+width)) {
        tex->empty_r -= tex->width - (left+width);
    } else {
        tex->empty_r = 0;
    }
    if (tex->empty_b > tex->height - (top+height)) {
        tex->empty_b -= tex->height - (top+height);
    } else {
        tex->empty_b = 0;
    }

    tex->width  = width;
    tex->height = height;
    tex->stride = new_stride;

    return 1;
}

/*----------------------------------*/

/**
 * shrink_texture:  Shrink the given texture to the given width and height.
 * The texture buffer is _not_ reallocated.
 *
 * [Parameters]
 *            tex: Texture to shrink
 *      new_width: New width for texture (pixels)
 *     new_height: New height for texture (pixels)
 * [Return value]
 *     Nonzero on success, zero on error
 */
static int shrink_texture(Texture *tex, int new_width, int new_height)
{
    const int new_stride = align_up(new_width, 4);
    void *tempbuf = malloc(new_stride * new_height * 4);
    if (!tempbuf) {
        fprintf(stderr, "Out of memory for shrink buffer (%d bytes)\n",
                new_width * new_height * 4);
    }

    ZoomInfo *zi = zoom_init(tex->width, tex->height,
                             new_width, new_height,
                             4, tex->stride*4, new_stride*4,
                             1, TCV_ZOOM_CUBIC_KEYS4);
    if (!zi) {
        fprintf(stderr, "zoom_init() failed\n");
        free(tempbuf);
        return 0;
    }
    zoom_process(zi, tex->pixels, tempbuf);
    zoom_free(zi);

    const int old_width  = tex->width;
    const int old_height = tex->height;
    tex->width  = new_width;
    tex->height = new_height;
    tex->stride = new_stride;
    memcpy(tex->pixels, tempbuf, new_stride * new_height * 4);
    free(tempbuf);

    /* Make sure we keep our 1-pixel transparent buffer at the new size. */
    while (new_width < old_width || new_height < old_height) {
        new_width *= 2;
        new_height *= 2;
        if (tex->empty_l > 0) {
            tex->empty_l = (tex->empty_l - 1) / 2;
        }
        if (tex->empty_r > 0) {
            tex->empty_r = (tex->empty_r - 1) / 2;
        }
        if (tex->empty_t > 0) {
            tex->empty_t = (tex->empty_t - 1) / 2;
        }
        if (tex->empty_b > 0) {
            tex->empty_b = (tex->empty_b - 1) / 2;
        }
    }

    return 1;
}

/*----------------------------------*/

/**
 * quantize_texture:  Convert the given texture to indexed-color 8bpp by
 * quantizing the color palette down to the 256 colors specified in
 * tex->palette.  The texture buffer is _not_ reallocated.
 *
 * [Parameters]
 *     tex: Texture to quantize
 * [Return value]
 *     Nonzero on success, zero on error
 */
static int quantize_texture(Texture *tex)
{
    unsigned int width  = tex->width;
    unsigned int height = tex->height;
    unsigned int stride = tex->stride;
    uint32_t *pixels_in = (uint32_t *)tex->pixels;
    uint8_t *pixels_out = tex->pixels;

    unsigned int level;
    for (level = 0; level <= tex->mipmaps; level++) {
        const int new_stride = align_up(stride, 16);
        if (!quantize_image(pixels_in, stride, pixels_out, new_stride,
                            width, height, tex->palette, 256, 0)) {
            fprintf(stderr, "quantize_image() failed for level %u\n", level);
            return 0;
        }
        if (level == 0) {
            tex->stride = new_stride;
        }
        pixels_in  += stride * height;
        pixels_out += new_stride * height;
        width  = (width +1) / 2;
        height = (height+1) / 2;
        stride = align_up(stride/2, 4);
    }

    tex->indexed = 1;
    return 1;
}

/*----------------------------------*/

/**
 * swizzle_texture:  Swizzle the given texture's pixel data.  The texture
 * buffer is _not_ reallocated (so it must be preallocated with a width
 * a multiple of 16 bytes and a height a multiple of 8 lines).
 *
 * [Parameters]
 *     tex: Texture to swizzle
 * [Return value]
 *     Nonzero on success, zero on error
 */
static int swizzle_texture(Texture *tex)
{
    unsigned int width  = tex->width;
    unsigned int height = tex->height;
    unsigned int stride = tex->stride;
    /* The pixels can be either 8bpp or 32bpp, but we process them as
     * 32bpp for speed.  (The swizzle block width is 16 bytes either way.) */
    uint32_t *pixels = (uint32_t *)tex->pixels;
    unsigned int stride_words = (tex->indexed ? stride/4 : stride);

    uint32_t *tempbuf = malloc(8 * (stride_words*4));
    if (!tempbuf) {
        fprintf(stderr, "Out of memory for temporary buffer (%u bytes)\n",
                8 * (stride_words*4));
        return 0;
    }

    unsigned int level;
    for (level = 0; level <= tex->mipmaps; level++) {
        const uint32_t *src = pixels;
        uint32_t *dest = pixels;

        int y;
        for (y = 0; y < tex->height; y += 8, src += 8*stride_words) {
            memcpy(tempbuf, src, 8 * (stride_words*4));
            const uint32_t *tempsrc = tempbuf;
            int x;
            for (x = 0; x < stride_words; x += 4, tempsrc += 4) {
                const uint32_t *linesrc = tempsrc;
                int line;
                for (line = 0; line < 8;
                     line++, linesrc += stride_words, dest += 4
                ) {
                    const uint32_t pixel0 = linesrc[0];
                    const uint32_t pixel1 = linesrc[1];
                    const uint32_t pixel2 = linesrc[2];
                    const uint32_t pixel3 = linesrc[3];
                    dest[0] = pixel0;
                    dest[1] = pixel1;
                    dest[2] = pixel2;
                    dest[3] = pixel3;
                }
            }
        }

        pixels += stride_words * height;
        width  = (width +1) / 2;
        height = (height+1) / 2;
        stride_words = align_up(stride_words/2, 4);
    }

    free(tempbuf);
    tex->swizzled = 1;
    return 1;
}

/*----------------------------------*/

/**
 * generate_texfile:  Generate a custom-format *.tex file for the given
 * texture.
 *
 * [Parameters]
 *          tex: Texture to write
 *     data_ret: Pointer to variable to receive data buffer
 *     size_ret: Pointer to variable to receive data size (bytes)
 * [Return value]
 *     Nonzero on success, zero on error
 */
static int generate_texfile(Texture *tex, void **data_ret, uint32_t *size_ret)
{
    TexFileHeader header;
    memset(&header, 0, sizeof(header));
    const uint32_t header_aligned_size = align_up(sizeof(header), 64);
    memcpy(header.magic, TEX_FILE_MAGIC, sizeof(header.magic));
    header.width    = s16_to_be(tex->width);
    header.height   = s16_to_be(tex->height);
    header.stride   = s16_to_be(tex->stride);
    header.indexed  = tex->indexed;
    header.swizzled = tex->swizzled;
    header.empty_l  = s16_to_be(tex->empty_l);
    header.empty_r  = s16_to_be(tex->empty_r);
    header.empty_t  = s16_to_be(tex->empty_t);
    header.empty_b  = s16_to_be(tex->empty_b);
    header.mipmaps  = tex->mipmaps;
    if (tex->indexed) {
        header.palette_offset = u32_to_be(header_aligned_size);
        header.pixels_offset  = u32_to_be(header_aligned_size + 256*4);
    } else {
        header.palette_offset = u32_to_be(0);
        header.pixels_offset  = u32_to_be(header_aligned_size);
    }

    uint8_t header_buf[align_up(sizeof(header), 64)];
    memset(header_buf, 0, sizeof(header_buf));
    memcpy(header_buf, &header, sizeof(header));

    uint32_t total_pixels = 0;
    unsigned int width = tex->width;
    unsigned int height = tex->height;
    unsigned int stride = tex->stride;
    unsigned int level;
    for (level = 0; level <= tex->mipmaps; level++) {
        const unsigned int data_height =
            tex->swizzled ? align_up(height, 8) : height;
        total_pixels += stride * data_height;
        width  = (width+1)/2;
        height = (height+1)/2;
        stride = align_up(stride/2, tex->indexed ? 16 : 4);
    }

    uint32_t texsize = sizeof(header_buf);
    if (tex->indexed) {
        texsize += 4*256 + total_pixels;
    } else {
        texsize += 4*total_pixels;
    }
    uint8_t *texdata = malloc(texsize);
    if (!texdata) {
        fprintf(stderr, "Out of memory\n");
        return 0;
    }
    *data_ret = texdata;
    *size_ret = texsize;

    memcpy(texdata, header_buf, sizeof(header_buf));
    texdata += sizeof(header_buf);

    if (tex->indexed) {
        memcpy(texdata, tex->palette, 4*256);
        texdata += 4*256;
    }

    memcpy(texdata, tex->pixels, (tex->indexed ? 1 : 4) * total_pixels);

    return 1;
}

/*-----------------------------------------------------------------------*/

/* Ogg Vorbis read callbacks. */

typedef struct ovFILE_ {
    const uint8_t *data;
    uint32_t size;
    uint32_t pos;
} ovFILE;

static size_t ogg_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    ovFILE *f = (ovFILE *)datasource;
    size_t toread = ubound(size, f->size - f->pos);
    if (toread > 0) {
        memcpy(ptr, f->data + f->pos, toread);
        f->pos += toread;
    }
    return toread;
}

static int ogg_seek(void *datasource, ogg_int64_t offset, int whence)
{
    ovFILE *f = (ovFILE *)datasource;
    switch (whence) {
        case SEEK_SET: f->pos = ubound(offset, f->size); break;
        case SEEK_CUR: f->pos = ubound(f->pos + offset, f->size); break;
        case SEEK_END: f->pos = ubound(f->size + offset, f->size); break;
    }
    return 0;
}

static long ogg_tell(void *datasource)
{
    ovFILE *f = (ovFILE *)datasource;
    return f->pos;
}

static const ov_callbacks ogg_callbacks = {
    .read_func  = ogg_read,
    .seek_func  = ogg_seek,
    .close_func = NULL,
    .tell_func  = ogg_tell,
};

/*----------------------------------*/

/* LAME message callbacks (all ignored). */
static void lame_error_callback(const char *format, va_list args) {}
static void lame_message_callback(const char *format, va_list args) {}
static void lame_debug_callback(const char *format, va_list args) {}

/*----------------------------------*/

/**
 * generate_mp3:  Convert an Ogg Vorbis audio file to MP3 format.
 *
 * [Parameters]
 *          oggdata: Ogg Vorbis data buffer
 *          oggsize: Ogg Vorbis data size (bytes)
 *      mp3data_ret: Pointer to variable to receive the MP3 data buffer
 *      mp3size_ret: Pointer to variable to receive the MP3 data size (bytes)
 *     progress_min: Initial progress value (0.0-1.0)
 *     progress_max: Final progress value (0.0-1.0)
 * [Return value]
 *     Nonzero on success, zero on failure
 */
static int generate_mp3(const void *oggdata, uint32_t oggsize,
                        void **mp3data_ret, uint32_t *mp3size_ret,
                        double progress_min, double progress_max)
{
    uint8_t *mp3data = NULL;
    uint32_t mp3size = 0;

    /* Initialize an Ogg Vorbis reader instance, and obtain the source
     * audio parameters. */

    ovFILE in = {.data = oggdata, .size = oggsize, .pos = 0};
    OggVorbis_File vf;
    if (ov_open_callbacks(&in, &vf, NULL, 0, ogg_callbacks) != 0) {
        fprintf(stderr, "ov_open() failed\n");
        goto error_return;
    }

    vorbis_info *info = ov_info(&vf, -1);
    if (!info) {
        fprintf(stderr, "ov_info() failed\n");
        goto error_close_vorbis;
    }
    if (info->channels != 1 && info->channels != 2) {
        fprintf(stderr, "Bad channel count %d\n", info->channels);
        goto error_close_vorbis;
    }

    /* Read the PCM data into memory. */

    uint32_t total_size = 0;
    uint8_t *pcm_buffer = NULL;
    uint32_t buffer_size = 0;
    for (;;) {
        const double done = (double)in.pos / (double)in.size;
        const double progress =
            progress_min + (done/6) * (progress_max - progress_min);
        SET_PROGRESS_AND_ITERATE(progress);

        if (total_size >= buffer_size) {
            buffer_size += 1000000;
            uint8_t *new_buffer = realloc(pcm_buffer, buffer_size);
            if (!new_buffer) {
                fprintf(stderr, "Out of memory for PCM data (%u bytes)\n",
                        buffer_size);
                goto error_free_pcm_buffer;
            }
            pcm_buffer = new_buffer;
        }

        int bitstream_unused;
        const int32_t nread = ov_read(
            &vf, (char *)(pcm_buffer + total_size), buffer_size - total_size,
            /*bigendianp*/ 0, /*word*/ 2, /*sgned*/ 1, &bitstream_unused
        );
        if (nread == 0 || nread == OV_EOF) {
            break;
        } else if (nread == OV_HOLE) {
            /* Don't print this, since it triggers on one of the real data
             * files (vox/naija_sunkencitydoor.ogg) and we don't want to
             * scare the user. */
            //fprintf(stderr, "Warning: Possible corrupt data at sample %u,"
            //        " continuing anyway\n", total_size / (info->channels*2));
        } else if (nread < 0) {
            fprintf(stderr, "Error %d decompressing Ogg Vorbis data\n", nread);
            goto error_free_pcm_buffer;
        } else {
            total_size += nread;
        }
    }

    const uint32_t num_samples = total_size / (info->channels * 2);

    /* Set up a LAME encoding context. */

    lame_global_flags *lame = lame_init();
    if (!lame) {
        fprintf(stderr, "lame_init() failed\n");
        goto error_free_pcm_buffer;
    }
    lame_set_errorf(lame, lame_error_callback);
    lame_set_msgf  (lame, lame_message_callback);
    lame_set_debugf(lame, lame_debug_callback);
    if (lame_set_in_samplerate(lame, info->rate) != 0) {
        fprintf(stderr, "lame_set_in_samplerate() failed\n");
        goto error_close_lame;
    }
    if (lame_set_num_channels(lame, info->channels) != 0) {
        fprintf(stderr, "lame_set_num_channels() failed\n");
        goto error_close_lame;
    }
    if (lame_set_bWriteVbrTag(lame, 1) != 0) {
        fprintf(stderr, "lame_set_bWriteVbrTag() failed\n");
        goto error_close_lame;
    }
    if (lame_set_mode(lame, info->channels==1 ? MONO : JOINT_STEREO) != 0) {
        fprintf(stderr, "lame_set_mode() failed\n");
        goto error_close_lame;
    }
    if (lame_set_VBR(lame, vbr_default) != 0) {
        fprintf(stderr, "lame_set_VBR() failed\n");
        goto error_close_lame;
    }
    if (lame_set_VBR_q(lame, 2) != 0) {
        fprintf(stderr, "lame_set_VBR_q() failed\n");
        goto error_close_lame;
    }
    if (lame_set_quality(lame, 1) != 0) {
        fprintf(stderr, "lame_set_quality() failed\n");
        goto error_close_lame;
    }
    if (lame_init_params(lame) != 0) {
        fprintf(stderr, "lame_init_params() failed\n");
        goto error_close_lame;
    }

    /* Encode the audio as MP3. */

    const int blocksize = 8192;
#define blocksize 8192 //Avoid a spurious error from GCC 4.5 (bugzilla #46028).
    uint8_t mp3_buffer[blocksize*5/4 + 7200];
#undef blocksize
    uint32_t pos;

    for (pos = 0; pos < num_samples; pos += blocksize) {

        const double done = (double)pos / (double)num_samples;
        const double progress =
            progress_min + ((1+5*done)/6) * (progress_max - progress_min);
        SET_PROGRESS_AND_ITERATE(progress);

        const uint32_t this_samples = ubound(blocksize, num_samples - pos);
        short *src = (short *)(pcm_buffer + pos * (info->channels*2));
        int res;
        if (info->channels == 1) {
            res = lame_encode_buffer(
                lame, src, src, this_samples, mp3_buffer, sizeof(mp3_buffer)
            );
        } else {
            res = lame_encode_buffer_interleaved(
                lame, src, this_samples, mp3_buffer, sizeof(mp3_buffer)
            );
        }
        if (res < 0) {
            fprintf(stderr, "LAME encoding error (%d)\n", res);
            goto error_close_lame;
        }

        if (res > 0) {
            uint8_t *mp3data_new = realloc(mp3data, mp3size + res);
            if (!mp3data_new) {
                fprintf(stderr, "Out of memory for MP3 data\n");
                goto error_close_lame;
            }
            mp3data = mp3data_new;
            memcpy(mp3data + mp3size, mp3_buffer, res);
            mp3size += res;
        }

    }

    int res = lame_encode_flush(lame, mp3_buffer, sizeof(mp3_buffer));
    if (res < 0) {
        fprintf(stderr, "LAME flush error (%d)\n", res);
        goto error_close_lame;
    }
    if (res > 0) {
        uint8_t *mp3data_new = realloc(mp3data, mp3size + res);
        if (!mp3data_new) {
            fprintf(stderr, "Out of memory for MP3 data\n");
            goto error_close_lame;
        }
        mp3data = mp3data_new;
        memcpy(mp3data + mp3size, mp3_buffer, res);
        mp3size += res;
    }

    /* Insert a Xing header at the beginning of the file so the engine
     * knows the true sample count (without padding). */

    res = lame_get_lametag_frame(lame, mp3_buffer, sizeof(mp3_buffer));
    if (res < 0) {
        fprintf(stderr, "LAME header genreation error (%d)\n", res);
        goto error_close_lame;
    }
    if (res > 0) {
        if (res > mp3size) {  // Should be impossible, but just in case
            res = mp3size;
        }
        memcpy(mp3data, mp3_buffer, res);
    }

    /* All done! */

    lame_close(lame);
    free(pcm_buffer);
    ov_clear(&vf);
    *mp3data_ret = mp3data;
    *mp3size_ret = mp3size;
    return 1;

    /* Error handling. */

  error_close_lame:
    lame_close(lame);
  error_free_pcm_buffer:
    free(pcm_buffer);
  error_close_vorbis:
    ov_clear(&vf);
  error_return:
    free(mp3data);
    return 0;
}

/*************************************************************************/

/**
 * build_eboot:  Write the EBOOT.PBP file for the game.
 *
 * [Parameters]
 *      in_path: Input (PSP data) directory path
 *     out_path: Output directory path (EBOOT.PBP is written into this
 *                  directory)
 * [Return value]
 *     None
 * [Notes]
 *     If an unrecoverable error occurs, this routine terminates the program.
 */
static void build_eboot(const char *in_path, const char *out_path)
{
    uint8_t *eboot_pbp;
    uint32_t eboot_pbp_size;
    struct {
        uint8_t magic[4];
        uint8_t version[4];
        uint8_t offset_param[4];
        uint8_t offset_icon0[4];
        uint8_t offset_icon1[4];
        uint8_t offset_pic0[4];
        uint8_t offset_pic1[4];
        uint8_t offset_snd0[4];
        uint8_t offset_prx[4];
        uint8_t offset_psar[4];
    } pbp_header;

    gtk_label_set_text(GTK_LABEL(ui_get_widget("label_build_status")),
                       "Building Aquaria for PSP...");
    current_file = "EBOOT.PBP";
    show_current_filename();
    gtk_label_set_text(GTK_LABEL(ui_get_widget("label_build_count")), "---");
    gtk_widget_set_sensitive(ui_get_widget("label_build_count_title"), FALSE);
    gtk_widget_set_sensitive(ui_get_widget("label_build_count"), FALSE);
    GTK_MAIN_ITERATION_OR_EXIT();

    void *icon0_png_data;
    uint32_t icon0_png_size;
    build_read_file(in_path, "ICON0.PNG", &icon0_png_data, &icon0_png_size);

    /* We align all file sizes to multiples of 4 bytes because that
     * drastically improves read performance from the Memory Stick. */
    const uint32_t param_sfo_size_aligned   = align_up(param_sfo_size, 4);
    const uint32_t icon0_png_size_aligned   = align_up(icon0_png_size, 4);
    const uint32_t aquaria_prx_size_aligned = align_up(aquaria_prx_size, 4);

    eboot_pbp_size = sizeof(pbp_header) + param_sfo_size_aligned
                                        + icon0_png_size_aligned
                                        + aquaria_prx_size_aligned;
    eboot_pbp = malloc(eboot_pbp_size);
    if (!eboot_pbp) {
        ui_oom();
    }

    memcpy(pbp_header.magic, "\0PBP", 4);
    memcpy(pbp_header.version, "\0\0\1\0", 4);
    uint32_t offset = sizeof(pbp_header);

    pbp_header.offset_param[0] = offset>> 0 & 0xFF;
    pbp_header.offset_param[1] = offset>> 8 & 0xFF;
    pbp_header.offset_param[2] = offset>>16 & 0xFF;
    pbp_header.offset_param[3] = offset>>24 & 0xFF;
    memcpy(eboot_pbp + offset, param_sfo, param_sfo_size);
    if (param_sfo_size_aligned > param_sfo_size) {
        memset(eboot_pbp + offset + param_sfo_size, 0,
               param_sfo_size_aligned - param_sfo_size);
    }
    offset += param_sfo_size_aligned;

    pbp_header.offset_icon0[0] = offset>> 0 & 0xFF;
    pbp_header.offset_icon0[1] = offset>> 8 & 0xFF;
    pbp_header.offset_icon0[2] = offset>>16 & 0xFF;
    pbp_header.offset_icon0[3] = offset>>24 & 0xFF;
    memcpy(eboot_pbp + offset, icon0_png_data, icon0_png_size);
    free(icon0_png_data);
    if (icon0_png_size_aligned > icon0_png_size) {
        memset(eboot_pbp + offset + icon0_png_size, 0,
               icon0_png_size_aligned - icon0_png_size);
    }
    offset += icon0_png_size_aligned;

    pbp_header.offset_icon1[0] = offset>> 0 & 0xFF;
    pbp_header.offset_icon1[1] = offset>> 8 & 0xFF;
    pbp_header.offset_icon1[2] = offset>>16 & 0xFF;
    pbp_header.offset_icon1[3] = offset>>24 & 0xFF;

    pbp_header.offset_pic0[0] = offset>> 0 & 0xFF;
    pbp_header.offset_pic0[1] = offset>> 8 & 0xFF;
    pbp_header.offset_pic0[2] = offset>>16 & 0xFF;
    pbp_header.offset_pic0[3] = offset>>24 & 0xFF;

    pbp_header.offset_pic1[0] = offset>> 0 & 0xFF;
    pbp_header.offset_pic1[1] = offset>> 8 & 0xFF;
    pbp_header.offset_pic1[2] = offset>>16 & 0xFF;
    pbp_header.offset_pic1[3] = offset>>24 & 0xFF;

    pbp_header.offset_snd0[0] = offset>> 0 & 0xFF;
    pbp_header.offset_snd0[1] = offset>> 8 & 0xFF;
    pbp_header.offset_snd0[2] = offset>>16 & 0xFF;
    pbp_header.offset_snd0[3] = offset>>24 & 0xFF;

    pbp_header.offset_prx[0] = offset>> 0 & 0xFF;
    pbp_header.offset_prx[1] = offset>> 8 & 0xFF;
    pbp_header.offset_prx[2] = offset>>16 & 0xFF;
    pbp_header.offset_prx[3] = offset>>24 & 0xFF;
    memcpy(eboot_pbp + offset, aquaria_prx, aquaria_prx_size);
    if (aquaria_prx_size_aligned > aquaria_prx_size) {
        memset(eboot_pbp + offset + aquaria_prx_size, 0,
               aquaria_prx_size_aligned - aquaria_prx_size);
    }
    offset += aquaria_prx_size;

    pbp_header.offset_psar[0] = offset>> 0 & 0xFF;
    pbp_header.offset_psar[1] = offset>> 8 & 0xFF;
    pbp_header.offset_psar[2] = offset>>16 & 0xFF;
    pbp_header.offset_psar[3] = offset>>24 & 0xFF;

    memcpy(eboot_pbp, &pbp_header, sizeof(pbp_header));
    build_write_file(out_path, "EBOOT.PBP", eboot_pbp, eboot_pbp_size);

    free(eboot_pbp);
}

/*************************************************************************/

typedef struct FileInfo_ {
    char *pathname;     // Package pathname for file
    char *realfile;     // Path to file on host filesystem (NULL if a script)
    unsigned int script_index;  // script_data[] index if realfile==NULL
    uint32_t size;      // Size of file (bytes)
    int index_entry;    // Location (array index) of file in package index
    int padding;        // Bytes of padding to insert before file data
} FileInfo;

#define NAME(a)  (namebuf + PKG_NAMEOFS(index[(a)].nameofs_flags))

static int stricmp_fileinfo(const void *a, const void *b)
    {return stricmp(((FileInfo *)a)->pathname, ((FileInfo *)b)->pathname);}

static void package_add_file(FileInfo **filelist_ptr, uint32_t *nfiles_ptr,
                             const char *fullpath, const char *localpath,
                             uint32_t filesize, uint32_t *namesize_ptr);
static void package_sort(PKGIndexEntry * const index, const uint32_t nfiles,
                         const char *namebuf,
                         const uint32_t left, const uint32_t right);

/*-----------------------------------------------------------------------*/

/**
 * build_package:  Build the aquaria.dat package file from the PSP data.
 *
 * [Parameters]
 *          in_path: Input (PSP data) directory path
 *         out_path: Output directory path (aquaria.dat is written into this
 *                      directory)
 *     progress_min: Initial progress value (0.0-1.0)
 *     progress_max: Final progress value (0.0-1.0)
 * [Return value]
 *     None
 * [Notes]
 *     If an unrecoverable error occurs, this routine terminates the program.
 */
static void build_package(const char *in_path, const char *out_path,
                          double progress_min, double progress_max)
{
    FileInfo *filelist;
    uint32_t nfiles;
    PKGIndexEntry *index;
    char *namebuf;
    uint32_t namesize;
    uint32_t i;

    /*
     * (1) Generate the package file list from the contents of the input
     *     directory and the built-in script data.
     */

    filelist = NULL;
    nfiles = 0;
    namesize = 0;
    build_scan_directory(&filelist, &nfiles, in_path, NULL,
                         (ScanDirCallback *)package_add_file, &namesize);

    for (i = 0; i < lenof(script_data); i++) {
        nfiles++;
        filelist = realloc(filelist, nfiles * sizeof(*filelist));
        if (!filelist) {
            ui_oom();
        }
        FileInfo *info = &filelist[nfiles-1];
        info->pathname = strdup(script_data[i].path);
        if (!info->pathname) {
            ui_oom();
        }
        info->realfile = NULL;
        info->script_index = i;
        info->size = strlen(script_data[i].data);
        namesize += strlen(info->pathname) + 1;
    }
    GTK_MAIN_ITERATION_OR_EXIT();

    /* Sort the file list so the user sees a nice alphabetical progression. */
    qsort(filelist, nfiles, sizeof(*filelist), stricmp_fileinfo);
    GTK_MAIN_ITERATION_OR_EXIT();

    /*
     * (2) Generate the package file index from the file list.
     */

    index = malloc(sizeof(*index) * nfiles);
    namebuf = malloc(namesize);
    if (!index || !namebuf) {
        ui_oom();
    }

    uint32_t nameofs = 0;
    const uint32_t database = sizeof(PKGHeader)
                            + (sizeof(PKGIndexEntry) * nfiles)
                            + namesize;
    uint32_t dataofs = database;
    for (i = 0; i < nfiles; i++) {
        index[i].hash          = pkg_hash(filelist[i].pathname);
        index[i].nameofs_flags = nameofs;
        index[i].datalen       = filelist[i].size;
        index[i].filesize      = filelist[i].size;
        const uint32_t namelen = strlen(filelist[i].pathname) + 1;
        memcpy(namebuf + nameofs, filelist[i].pathname, namelen);
        nameofs += namelen;
        /* We add padding before each file as necessary to align the
         * file's data offset to a multiple of 4 bytes; this drastically
         * improves read performance. */
        if (dataofs % 4 != 0) {
            filelist[i].padding = 4 - (dataofs % 4);
            dataofs += filelist[i].padding;
        } else {
            filelist[i].padding = 0;
        }
        index[i].offset = dataofs;
        dataofs += filelist[i].size;
    }

    const uint32_t datasize = dataofs - database;

    package_sort(index, nfiles, namebuf, 0, nfiles-1);
    GTK_MAIN_ITERATION_OR_EXIT();

    for (i = 0; i < nfiles; i++) {
        filelist[i].index_entry = -1;
        const uint32_t hash = pkg_hash(filelist[i].pathname);
        uint32_t j;
        for (j = 0; j < nfiles; j++) {
            if (index[j].hash == hash
             && stricmp(filelist[i].pathname, NAME(j)) == 0
            ) {
                filelist[i].index_entry = j;
                break;
            }
        }
        if (filelist[i].index_entry < 0) {
            fprintf(stderr, "File %s lost from index!\n",
                    filelist[i].pathname);
            ui_show_error("Internal error: file index corrupted! Aborting.\n");
            exit(1);
        }
    }

    /*
     * (3) Open the package file and write the header.
     */

    FILE *pkg;

    char *pkg_path = g_build_filename(out_path, "aquaria.dat", NULL);
    if (!pkg_path) {
        ui_oom();
    }
    while (!(pkg = g_fopen(pkg_path, "wb"))) {
        char buf[1000];
        snprintf(buf, sizeof(buf), "Unable to create file: %s",
                 strerror(errno));
        build_report_error(pkg_path, 1, &((GError){.message = buf}));
    }

    PKGHeader header;
    memset(&header, 0, sizeof(header));
    memcpy(header.magic, PKG_MAGIC, 4);
    header.header_size = sizeof(PKGHeader);
    header.entry_size  = sizeof(PKGIndexEntry);
    header.entry_count = nfiles;
    header.name_size   = namesize;
    PKG_HEADER_SWAP_BYTES(header);
    while (fwrite(&header, sizeof(header), 1, pkg) != 1) {
        build_report_error(pkg_path, 1,
                           &((GError){.message = strerror(errno)}));
        fseek(pkg, 0, SEEK_SET);
    }

    PKG_INDEX_SWAP_BYTES(index, nfiles);
    while (fwrite(index, sizeof(*index), nfiles, pkg) != nfiles) {
        build_report_error(pkg_path, 1,
                           &((GError){.message = strerror(errno)}));
        fseek(pkg, sizeof(header), SEEK_SET);
    }
    PKG_INDEX_SWAP_BYTES(index, nfiles);  // So we can use it below

    while (fwrite(namebuf, namesize, 1, pkg) != 1) {
        build_report_error(pkg_path, 1,
                           &((GError){.message = strerror(errno)}));
        fseek(pkg, sizeof(header) + sizeof(*index) * nfiles, SEEK_SET);
    }

    GTK_MAIN_ITERATION_OR_EXIT();

    /*
     * (4) Copy all files' data into the package file.
     */

    gtk_widget_set_sensitive(ui_get_widget("label_build_count_title"), TRUE);
    gtk_widget_set_sensitive(ui_get_widget("label_build_count"), TRUE);

    uint32_t datadone = 0;
    for (i = 0; i < nfiles; i++) {
        current_file = filelist[i].pathname;
        show_current_filename();
        char countbuf[32];
        snprintf(countbuf, sizeof(countbuf), "%u/%u", i+1, nfiles);
        gtk_label_set_text(GTK_LABEL(ui_get_widget("label_build_count")),
                           countbuf);

        /* We can't rely on either the file count or the data size alone
         * to measure progress in terms of real time, since both have an
         * effect; the *_weight constants here are a rough guesstimate of
         * the expense of file opens/closes versus reads/writes. */
        const double file_weight = 0.85;
        const double data_weight = 0.15;
        const double file_fraction = (double)i / (double)nfiles;
        const double data_fraction = (double)datadone / (double)datasize;
        const double fraction = file_fraction * file_weight
                              + data_fraction * data_weight;
        const double progress = progress_min
                              + (progress_max - progress_min) * fraction;
        SET_PROGRESS_AND_ITERATE(progress);

        uint32_t filepos = ftell(pkg);
        if (filelist[i].padding > 0) {
            static const uint8_t padbuf[4] = {0,0,0,0};
            while (fwrite(padbuf, filelist[i].padding, 1, pkg) != 1) {
                build_report_error(pkg_path, 1,
                                   &((GError){.message = strerror(errno)}));
                fseek(pkg, filepos, SEEK_SET);
            }
            filepos = ftell(pkg);
        }

        if (filelist[i].size == 0) {
            continue;
        }

        void *filedata;
        uint32_t readlen;
        if (filelist[i].realfile) {
            while (build_read_file(NULL, filelist[i].realfile,
                                   &filedata, &readlen),
                   readlen != filelist[i].size
            ) {
                char buf[1000];
                snprintf(buf, sizeof(buf), "File size changed (got %u,"
                         " expected %u)", readlen, filelist[i].size);
                build_report_error(filelist[i].realfile, 1,
                                   &((GError){.message = buf}));
                g_free(filedata);
            }
        } else {
            filedata = (void *)script_data[filelist[i].script_index].data;
        }

        while (fwrite(filedata, filelist[i].size, 1, pkg) != 1) {
            build_report_error(pkg_path, 1,
                               &((GError){.message = strerror(errno)}));
            fseek(pkg, filepos, SEEK_SET);
        }

        if (filelist[i].realfile) {
            g_free(filedata);
        }

        datadone += filelist[i].size;
    }

    fclose(pkg);

    /*
     * (5) Free all resources used.
     */

    g_free(pkg_path);
    free(namebuf);
    free(index);
    for (i = 0; i < nfiles; i++) {
        free(filelist[i].pathname);
        free(filelist[i].realfile);
    }
    free(filelist);
}

/*-----------------------------------------------------------------------*/

/**
 * package_add_file:  Directory scan callback which adds files to the
 * package file list.  *.lua files are ignored.
 *
 * [Parameters]
 *     filelist_ptr: Pointer to variable holding the file list array
 *       nfiles_ptr: Pointer to variable holding the number of files
 *         fullpath: Path of file in host filesystem
 *        localpath: Corresponding directory path for package file
 *         filesize: Size of file (bytes)
 *     namesize_ptr: Pointer to variable holding the required name buffer size
 * [Return value]
 *     None
 */
static void package_add_file(FileInfo **filelist_ptr, uint32_t *nfiles_ptr,
                             const char *fullpath, const char *localpath,
                             uint32_t filesize, uint32_t *namesize_ptr)
{
    if (strlen(localpath) >= 4
     && stricmp(localpath + strlen(localpath)-4, ".lua") == 0
    ) {
        return;
    }

    (*nfiles_ptr)++;
    *filelist_ptr =
        realloc(*filelist_ptr, *nfiles_ptr * sizeof(**filelist_ptr));
    if (!*filelist_ptr) {
        ui_oom();
    }
    FileInfo *info = &(*filelist_ptr)[(*nfiles_ptr)-1];
    info->pathname = strdup(localpath);
    info->realfile = strdup(fullpath);
    info->size     = filesize;
    (*namesize_ptr) += strlen(localpath) + 1;
    if (!info->pathname || !info->realfile) {
        ui_oom();
    }
}

/*-----------------------------------------------------------------------*/

/**
 * package_sort:  Sort an array of PKGIndexEntry elements using the
 * quicksort algorithm.
 *
 * [Parameters]
 *       index: PKGIndexEntry array pointer
 *      nfiles: Number of array elements
 *     namebuf: Name buffer pointer
 *        left: Index of first element to sort in this call
 *       right: Index of last element to sort in this call
 * [Return value]
 *     None
 */
static void package_sort(PKGIndexEntry * const index, const uint32_t nfiles,
                         const char *namebuf,
                         const uint32_t left, const uint32_t right)
{
    #define LESS(a)  (index[(a)].hash < pivot_hash   \
                   || (index[(a)].hash == pivot_hash \
                       && stricmp(NAME(a), pivot_name) < 0))
    #define SWAP(a,b) do {        \
        typeof(index[(a)]) tmp;   \
        tmp = index[(a)];         \
        index[(a)] = index[(b)];  \
        index[(b)] = tmp;         \
    } while (0)

    if (left >= right) {
        return;
    }
    const int pivot = (left + right + 1) / 2;
    const uint32_t pivot_hash = index[pivot].hash;
    const char * const pivot_name = NAME(pivot);
    SWAP(pivot, right);

    int store = left;
    while (store < right && LESS(store)) {
        store++;
    }
    int i;
    for (i = store+1; i <= right-1; i++) {
        if (LESS(i)) {
            SWAP(i, store);
            store++;
        }
    }
    SWAP(right, store);  // right == old pivot

    if (store > 0) {
        package_sort(index, nfiles, namebuf, left, store-1);
    }
    package_sort(index, nfiles, namebuf, store+1, right);

    #undef LESS
    #undef SWAP
}

/*-----------------------------------------------------------------------*/

#undef NAME

/*************************************************************************/
/******************** Miscellaneous utility functions ********************/
/*************************************************************************/

/* PNG read/write callbacks and associated data structure. */

typedef struct pngRFILE_ {
    const uint8_t *data;
    uint32_t size;
    uint32_t pos;
} pngRFILE;
static void png_read(png_structp png, png_bytep data, png_size_t length) {
    pngRFILE *f = (pngRFILE *)png_get_io_ptr(png);
    size_t toread = ubound(length, f->size - f->pos);
    if (toread > 0) {
        memcpy(data, f->data + f->pos, toread);
        f->pos += toread;
    }
}

typedef struct pngWFILE_ {
    uint8_t *data;
    uint32_t size;
} pngWFILE;
static void png_write(png_structp png, png_bytep data, png_size_t length) {
    pngWFILE *f = (pngWFILE *)png_get_io_ptr(png);
    uint8_t *new_data = realloc(f->data, f->size + length);
    if (!new_data) {
        png_error(png, "Out of memory");
        return;
    }
    f->data = new_data;
    memcpy(f->data + f->size, data, length);
    f->size += length;
}
static void png_flush(png_structp png) {}

/*----------------------------------*/

/* PNG warning/error callbacks. */

static void png_warning_callback(png_structp png, const char *message) {}

static void png_error_callback(png_structp png, const char *message) {
    fprintf(stderr, "libpng error: %s\n", message);
    longjmp(png_jmpbuf, 1);
}

/*-----------------------------------------------------------------------*/

/**
 * parse_png:  Parse a PNG file into a Texture data structure.
 *
 * [Parameters]
 *     data: PNG file data
 *     size: PNG file size (bytes)
 * [Return value]
 *     Texture, or NULL on error
 */
static Texture *parse_png(const uint8_t *data, uint32_t size)
{
    /* We have to be able to free these on error, so we need volatile
     * declarations. */
    png_structp volatile png_volatile = NULL;
    png_infop volatile info_volatile = NULL;
    Texture * volatile texture_volatile = NULL;
    void * volatile row_buffer_volatile = NULL;

    if (setjmp(png_jmpbuf) != 0) {
        /* libpng jumped back here with an error, so return the error. */
      error:  // Let's reuse it for our own error handling, too.
        free(row_buffer_volatile);
        free(texture_volatile);
        png_destroy_read_struct((png_structpp)&png_volatile,
                                (png_infopp)&info_volatile, NULL);
        return NULL;
    }


    /* Set up the PNG reader instance. */

    png_structp png = png_create_read_struct(
        PNG_LIBPNG_VER_STRING,
        NULL, png_error_callback, png_warning_callback
    );
    png_volatile = png;
    png_infop info = png_create_info_struct(png);
    info_volatile = info;
    pngRFILE in = {.data = data, .size = size, .pos = 0};
    png_set_read_fn(png, &in, png_read);

    /* Read the image information. */

    png_read_info(png, info);
    const unsigned int width      = png_get_image_width(png, info);
    const unsigned int height     = png_get_image_height(png, info);
    const unsigned int bit_depth  = png_get_bit_depth(png, info);
    const unsigned int color_type = png_get_color_type(png, info);
    if (png_get_interlace_type(png, info) != PNG_INTERLACE_NONE) {
        DMSG("Interlaced images not supported");
        goto error;
    }
    if (bit_depth < 8) {
        DMSG("Bit depth %d not supported", bit_depth);
        goto error;
    }

    /* Set up image transformation parameters. */

    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);
    } else if (color_type == PNG_COLOR_TYPE_GRAY) {
        png_set_gray_to_rgb(png);
    }
    if (bit_depth == 16) {
        png_set_strip_16(png);
    }
    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
    } else if (color_type == PNG_COLOR_TYPE_RGB
            || color_type == PNG_COLOR_TYPE_PALETTE
            || color_type == PNG_COLOR_TYPE_GRAY
    ) {
        png_set_add_alpha(png, 0xFF, PNG_FILLER_AFTER);
    }
    png_read_update_info(png, info);

    /* Create the texture structure. */

    Texture *texture;
    const uint32_t tex_width    = width;
    const uint32_t tex_height   = height;
    const uint32_t alloc_width  = align_up(width, 4);
    const uint32_t alloc_height = align_up(height, 8);
    const uint32_t struct_size  = align_up(sizeof(*texture), 64);
    texture = malloc(struct_size + (alloc_width * alloc_height * 4));
    if (!texture) {
        DMSG("Out of memory for texture (%ux%u, %u bytes)", tex_width,
             tex_height, struct_size + (alloc_width * alloc_height * 4));
        goto error;
    }
    texture->width    = tex_width;
    texture->height   = tex_height;
    texture->stride   = alloc_width;
    texture->indexed  = 0;
    texture->swizzled = 0;
    texture->empty_l  = tex_width;
    texture->empty_r  = tex_width;
    texture->empty_t  = tex_height;
    texture->empty_b  = tex_height;
    texture->mipmaps  = 0;
    texture->pixels   = (uint8_t *)texture + struct_size;

    texture_volatile = texture;

    /* Read the image in one row at a time, updating the empty_[lrtb]
     * fields as we go. */

    uint8_t *row_buffer;
    uint32_t rowbytes = png_get_rowbytes(png, info);
    row_buffer = malloc(rowbytes);
    if (!row_buffer) {
        DMSG("Out of memory for pixel read buffer (%u bytes)", rowbytes);
        goto error;
    }
    row_buffer_volatile = row_buffer;

    uint8_t *dest = texture->pixels;
    unsigned int y;
    for (y = 0; y < height; y++, dest += texture->stride * 4) {
        png_read_row(png, row_buffer, NULL);
        memcpy(dest, row_buffer, rowbytes);
        int whole_row_empty = 1;
        unsigned int x;
        for (x = 0; x < width; x++) {
            if (dest[x*4+3] != 0) {
                whole_row_empty = 0;
                texture->empty_l = min(texture->empty_l, x);
                texture->empty_r = min(texture->empty_r, tex_width - (x+1));
            }
        }
        if (!whole_row_empty) {
            texture->empty_t = min(texture->empty_t, y);
            texture->empty_b = min(texture->empty_b, tex_height - (y+1));
        }
    }

    row_buffer_volatile = NULL;
    free(row_buffer);

    /* Decrement all the empty_[lrtb] fields by one (if they're not zero
     * already) to provide a 1-pixel transparent buffer around the texture
     * and ensure that graphics hardware rounding doesn't cut anything off. */

    if (texture->empty_l > 0) {
        texture->empty_l--;
    }
    if (texture->empty_r > 0) {
        texture->empty_r--;
    }
    if (texture->empty_t > 0) {
        texture->empty_t--;
    }
    if (texture->empty_b > 0) {
        texture->empty_b--;
    }

    /* Done!  Close down the PNG reader and return success. */

    png_read_end(png, NULL);
    png_destroy_read_struct(&png, &info, NULL);
    return texture;
}

/*-----------------------------------------------------------------------*/

/**
 * parse_png:  Generate a PNG file from a Texture data structure.  The
 * returned buffer should be free()d when no longer needed.
 *
 * [Parameters]
 *      texture: Image data
 *     data_ret: Pointer to variable to receive PNG file data buffer
 *     size_ret: Pointer to variable to receive PNG file size (bytes)
 * [Return value]
 *     Nonzero on success, zero on error
 */
static int create_png(Texture *texture, void **data_ret, uint32_t *size_ret)
{
    /* As with parse_png(), these need to be volatile. */
    png_structp volatile png_volatile = NULL;
    png_infop volatile info_volatile = NULL;
    uint8_t ** volatile data_ptr_volatile = NULL;
    uint8_t ** volatile row_pointers_volatile = NULL;

    if (setjmp(png_jmpbuf) != 0) {
        /* libpng (or we) jumped back with an error, so return the error. */
      error:
        free(row_pointers_volatile);
        free(*data_ptr_volatile);
        png_destroy_write_struct((png_structpp)&png_volatile,
                                 (png_infopp)&info_volatile);
        return 0;
    }


    /* Make sure we have standard RGBA data (we don't support indexed or
     * swizzled data here). */

    if (texture->indexed || texture->swizzled) {
        fprintf(stderr, "Invalid texture flags (indexed=%d swizzled=%d)\n",
                texture->indexed, texture->swizzled);
        goto error;
    }

    /* Set up the PNG writer instance. */

    png_structp png = png_create_write_struct(
        PNG_LIBPNG_VER_STRING,
        NULL, png_error_callback, png_warning_callback
    );
    png_volatile = png;
    png_infop info = png_create_info_struct(png);
    info_volatile = info;
    pngWFILE out = {.data = NULL, .size = 0};
    data_ptr_volatile = &out.data;
    png_set_write_fn(png, &out, png_write, png_flush);

    /* Set up image encoding parameters. */

    png_set_IHDR(png, info, texture->width, texture->height, 8,
                 PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_compression_level(png, Z_BEST_COMPRESSION);
    png_set_compression_mem_level(png, 9);
    png_set_compression_window_bits(png, 15);

    /* Create the PNG data. */

    uint8_t **row_pointers = malloc(texture->height * sizeof(uint8_t *));
    if (!row_pointers) {
        fprintf(stderr, "Out of memory for %d row pointers\n",texture->height);
        goto error;
    }
    row_pointers_volatile = row_pointers;
    int y;
    for (y = 0; y < texture->height; y++) {
        row_pointers[y] = &texture->pixels[y * (texture->stride * 4)];
    }
    png_set_rows(png, info, row_pointers);

    png_write_png(png, info,
#ifdef IS_LITTLE_ENDIAN
                  PNG_TRANSFORM_IDENTITY,
#else
                  PNG_TRANSFORM_BGR | PNG_TRANSFORM_SWAP_ALPHA,
#endif
                  NULL);

    row_pointers_volatile = NULL;
    free(row_pointers);

    /* Done!  Close down the PNG writer and return success. */

    png_write_end(png, NULL);
    png_destroy_write_struct(&png, &info);
    *data_ret = out.data;
    *size_ret = out.size;
    return 1;
}

/*************************************************************************/

/**
 * build_scan_directory:  Recursively scan a directory, calling the given
 * callback function for each regular file found.
 *
 * [Parameters]
 *     filelist_ptr: Pointer to variable holding the file list array
 *       nfiles_ptr: Pointer to variable holding the number of files
 *         basepath: Base path of directory to scan
 *           subdir: Subdirectory of basepath to scan (NULL to scan "basepath")
 *         callback: Pointer to function to call for each file found
 *         userdata: Opaque parameter passed to callback
 * [Return value]
 *     None
 */
static void build_scan_directory(void *filelist_ptr, uint32_t *nfiles_ptr,
                                 const char *basepath, const char *subdir,
                                 ScanDirCallback *callback, void *userdata)
{
    GError *error;
    GDir *dir;

    char *dirpath = g_build_filename(basepath, subdir, NULL);
    while (error = NULL, (dir = g_dir_open(dirpath, 0, &error)) == NULL) {
        build_report_error(dirpath, 0, error);
    }

    const char *name;
    while ((name = g_dir_read_name(dir)) != NULL) {
        char *fullpath = g_build_filename(dirpath, name, NULL);
        char *localpath;
        if (subdir) {
            localpath = g_build_filename(subdir, name, NULL);
        } else {
            localpath = g_strdup(name);
        }
        if (!fullpath || !localpath) {
            ui_oom();
        }

        if (g_file_test(fullpath, G_FILE_TEST_IS_DIR)) {
            build_scan_directory(filelist_ptr, nfiles_ptr, basepath, localpath,
                                 callback, userdata);

        } else if (g_file_test(fullpath, G_FILE_TEST_IS_REGULAR)) {
#ifdef _WIN32
            struct _g_stat_struct st;
#else
            struct stat st;
#endif
            while (g_stat(fullpath, &st) != 0) {
                char buf[1000];
                snprintf(buf, sizeof(buf), "Unable to read file attributes: %s",
                         strerror(errno));
                build_report_error(fullpath, 0, &((GError){.message = buf}));
            }
            (*callback)(filelist_ptr, nfiles_ptr, fullpath, localpath,
                        st.st_size, userdata);
        }

        g_free(fullpath);
        g_free(localpath);
    }  // while ((name = g_dir_read_name(dir)) != NULL)

    g_free(dirpath);
}

/*-----------------------------------------------------------------------*/

/**
 * build_read_file:  Read a file from disk, with retry support.
 *
 * [Parameters]
 *        directory: Parent directory of filename (may be NULL)
 *         filename: Name of file to write (may contain directory components)
 *     contents_ret: Pointer to variable to receive the file's data
 *       length_ret: Pointer to variable to receive the file's length
 * [Return value]
 *     None
 */
static void build_read_file(const char *directory, const char *filename,
                            void **contents_ret, uint32_t *length_ret)
{
    char *path;
    if (directory) {
        path = g_build_filename(directory, filename, NULL);
    } else {
        path = g_strdup(filename);
    }
    if (!path) {
        ui_oom();
    }

    gchar *contents;
    gsize length;
    GError *error;
    while (error = NULL, !g_file_get_contents(path, &contents, &length,
                                              &error)) {
        build_report_error(path, 0, error);
    }

    *contents_ret = contents;
    *length_ret = length;
    g_free(path);
}

/*-----------------------------------------------------------------------*/

/**
 * build_write_file:  Write a file to disk, with automatic parent directory
 * creation and retry support.
 *
 * [Parameters]
 *     directory: Parent directory of filename (may be NULL)
 *      filename: Name of file to write (may contain directory components)
 *      contents: Data to write
 *        length: Length of data to write, or -1 if the data is null-terminated
 * [Return value]
 *     None
 */
static void build_write_file(const char *directory, const char *filename,
                             const void *contents, int32_t length)
{
    char *path, *parent;
    if (directory) {
        path = g_build_filename(directory, filename, NULL);
    } else {
        path = g_strdup(filename);
    }
    if (!path) {
        ui_oom();
    }
    parent = g_path_get_dirname(path);
    if (!parent) {
        ui_oom();
    }

    while (g_mkdir_with_parents(parent, 0777) != 0) {
        char buf[1000];
        snprintf(buf, sizeof(buf), "Unable to create parent directory: %s",
                 strerror(errno));
        build_report_error(path, 1, &((GError){.message = buf}));
    }

    GError *error;
    while (error = NULL, !g_file_set_contents(path, contents, length, &error)) {
        build_report_error(path, 1, error);
    }

    g_free(path);
    g_free(parent);
}

/*-----------------------------------------------------------------------*/

/**
 * build_report_error:  Report an I/O error to the user, and wait for an
 * "abort" or "retry" response.  If the user selects "abort", this routine
 * exits the program with an unsuccessful exit code.  Closing the dialog
 * window is considered a "retry" response.
 *
 * [Parameters]
 *         path: Pathname of file on which the error occurred
 *     is_write: Nonzero if a write operation, zero if a read operations
 *        error: Glib error
 * [Return value]
 *     None
 */
static void build_report_error(const char *path, int is_write,
                               const GError *error)
{
    GtkWidget *message = ui_get_widget("message_io_error");
    gtk_window_set_transient_for(GTK_WINDOW(message),
                                 GTK_WINDOW(ui_get_widget("main_window")));

    char *default_secondary_text = NULL;
    g_object_get(message, "secondary-text", &default_secondary_text, NULL);
    gtk_message_dialog_format_secondary_text(
        GTK_MESSAGE_DIALOG(message), "While %s %s: %s\n%s",
        is_write ? "writing" : "reading", path, error->message,
        default_secondary_text ? default_secondary_text : ""
    );

    const int response = gtk_dialog_run(GTK_DIALOG(message));
    if (response == GTK_RESPONSE_CLOSE) {
        exit(1);
    }
    gtk_widget_hide(message);

    gtk_message_dialog_format_secondary_text(
        GTK_MESSAGE_DIALOG(message), "%s",
        default_secondary_text ? default_secondary_text : ""
    );
    g_free(default_secondary_text);
}

/*************************************************************************/

/**
 * show_current_file:  Show the current filename in the build frame, unless
 * the "hide filenames" checkbox is checked, in which case the filename
 * will be hidden.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void show_current_filename(void)
{
    if (!in_build) {
        return;
    }

    GtkWidget *checkbox = ui_get_widget("check_hide_filenames");
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbox))) {
        gtk_label_set_text(GTK_LABEL(ui_get_widget("label_build_file")), "---");
        gtk_widget_set_sensitive(ui_get_widget("label_build_file_title"),FALSE);
        gtk_widget_set_sensitive(ui_get_widget("label_build_file"), FALSE);
    } else {
        gtk_label_set_text(GTK_LABEL(ui_get_widget("label_build_file")),
                           current_file);
        gtk_widget_set_sensitive(ui_get_widget("label_build_file_title"), TRUE);
        gtk_widget_set_sensitive(ui_get_widget("label_build_file"), TRUE);
    }
}

/*************************************************************************/
/*************************************************************************/

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
