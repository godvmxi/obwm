/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-

   openbox.c for the Openbox window manager
   Copyright (c) 2006        Mikael Magnusson
   Copyright (c) 2003-2007   Dana Jansens

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   See the COPYING file for a copy of the GNU General Public License.
*/

#include "debug.h"
#include "openbox.h"
#include "session.h"
#include "dock.h"
#include "modkeys.h"
#include "event.h"
#include "menu.h"
#include "client.h"
#include "xerror.h"
#include "prop.h"
#include "screen.h"
#include "actions.h"
#include "startupnotify.h"
#include "focus.h"
#include "focus_cycle.h"
#include "focus_cycle_indicator.h"
#include "focus_cycle_popup.h"
#include "moveresize.h"
#include "frame.h"
#include "framerender.h"
#include "keyboard.h"
#include "mouse.h"
#include "extensions.h"
#include "menuframe.h"
#include "grab.h"
#include "group.h"
#include "config.h"
#include "ping.h"
#include "mainloop.h"
#include "prompt.h"
#include "gettext.h"
#include "parser/parse.h"
#include "render/render.h"
#include "render/theme.h"

#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif
#ifdef HAVE_SIGNAL_H
#  include <signal.h>
#endif
#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif
#ifdef HAVE_LOCALE_H
#  include <locale.h>
#endif
#ifdef HAVE_SYS_STAT_H
#  include <sys/stat.h>
#  include <sys/types.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#  include <sys/types.h>
#  include <sys/wait.h>
#endif
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <errno.h>

#include <X11/cursorfont.h>
#if USE_XCURSOR
#include <X11/Xcursor/Xcursor.h>
#endif

#include <X11/Xlib.h>
#include <X11/keysym.h>

RrInstance   *ob_rr_inst;
RrImageCache *ob_rr_icons;
RrTheme      *ob_rr_theme;
ObMainLoop   *ob_main_loop;
Display      *ob_display;
gint          ob_screen;
gboolean      ob_replace_wm = FALSE;
gboolean      ob_sm_use = TRUE;
gchar        *ob_sm_id = NULL;
gchar        *ob_sm_save_file = NULL;
gboolean      ob_sm_restore = TRUE;
gboolean      ob_debug_xinerama = FALSE;

static ObState   state;
static gboolean  xsync = FALSE;
static gboolean  reconfigure = FALSE;
static gboolean  restart = FALSE;
static gchar    *restart_path = NULL;
static Cursor    cursors[OB_NUM_CURSORS];
static KeyCode  *keys[OB_NUM_KEYS];
static gint      exitcode = 0;
static guint     remote_control = 0;
static gboolean  being_replaced = FALSE;
static gchar    *config_file = NULL;

static void signal_handler(gint signal, gpointer data);
static void remove_args(gint *argc, gchar **argv, gint index, gint num);
static void parse_env();
static void parse_args(gint *argc, gchar **argv);
static Cursor load_cursor(const gchar *name, guint fontval);

gint main(gint argc, gchar **argv)
{
    gchar *program_name;

    ob_set_state(OB_STATE_STARTING);
    openlog("OPENBOX",LOG_CONS|LOG_PID,LOG_USER);
	syslog(LOG_INFO,"openbox start");
    /* initialize the locale */
    if (!setlocale(LC_ALL, ""))
        g_message("Couldn't set locale from environment.");
    bindtextdomain(PACKAGE_NAME, LOCALEDIR);
    bind_textdomain_codeset(PACKAGE_NAME, "UTF-8");
    textdomain(PACKAGE_NAME);

    if (chdir(g_get_home_dir()) == -1)
        g_message(_("Unable to change to home directory \"%s\": %s"),
                  g_get_home_dir(), g_strerror(errno));

    /* parse the command line args, which can change the argv[0] */
    parse_args(&argc, argv);
    /* parse the environment variables */
    parse_env();

    program_name = g_path_get_basename(argv[0]);
    g_set_prgname(program_name);

    if (!remote_control) {
        parse_paths_startup();

        session_startup(argc, argv);
    }

    ob_display = XOpenDisplay(NULL);
    if (ob_display == NULL)
        ob_exit_with_error(_("Failed to open the display from the DISPLAY environment variable."));
    if (fcntl(ConnectionNumber(ob_display), F_SETFD, 1) == -1)
        ob_exit_with_error("Failed to set display as close-on-exec");

    if (remote_control) {
        prop_startup();

        /* Send client message telling the OB process to:
         * remote_control = 1 -> reconfigure
         * remote_control = 2 -> restart */
        PROP_MSG(RootWindow(ob_display, ob_screen),
                 ob_control, remote_control, 0, 0, 0);
        XCloseDisplay(ob_display);
        exit(EXIT_SUCCESS);
    }

    ob_main_loop = ob_main_loop_new(ob_display);

    /* set up signal handler */
    ob_main_loop_signal_add(ob_main_loop, SIGUSR1, signal_handler, NULL, NULL);
    ob_main_loop_signal_add(ob_main_loop, SIGUSR2, signal_handler, NULL, NULL);
    ob_main_loop_signal_add(ob_main_loop, SIGTERM, signal_handler, NULL, NULL);
    ob_main_loop_signal_add(ob_main_loop, SIGINT, signal_handler, NULL, NULL);
    ob_main_loop_signal_add(ob_main_loop, SIGHUP, signal_handler, NULL, NULL);
    ob_main_loop_signal_add(ob_main_loop, SIGPIPE, signal_handler, NULL, NULL);
    ob_main_loop_signal_add(ob_main_loop, SIGCHLD, signal_handler, NULL, NULL);
    ob_main_loop_signal_add(ob_main_loop, SIGTTIN, signal_handler, NULL,NULL);
    ob_main_loop_signal_add(ob_main_loop, SIGTTOU, signal_handler, NULL,NULL);

    ob_screen = DefaultScreen(ob_display);

    ob_rr_inst = RrInstanceNew(ob_display, ob_screen);
    if (ob_rr_inst == NULL)
        ob_exit_with_error(_("Failed to initialize the obrender library."));
    /* Saving 3 resizes of an RrImage makes a lot of sense for icons, as there
       are generally 3 icon sizes needed: the titlebar icon, the menu icon,
       and the alt-tab icon
    */
    ob_rr_icons = RrImageCacheNew(3);

    XSynchronize(ob_display, xsync);

    /* check for locale support */
    if (!XSupportsLocale())
        g_message(_("X server does not support locale."));
    if (!XSetLocaleModifiers(""))
        g_message(_("Cannot set locale modifiers for the X server."));

    /* set our error handler */
    XSetErrorHandler(xerror_handler);

    /* set the DISPLAY environment variable for any lauched children, to the
       display we're using, so they open in the right place. */
    setenv("DISPLAY", DisplayString(ob_display), TRUE);

    /* create available cursors */
    cursors[OB_CURSOR_NONE] = None;
    cursors[OB_CURSOR_POINTER] = load_cursor("left_ptr", XC_left_ptr);
    cursors[OB_CURSOR_BUSYPOINTER] = load_cursor("left_ptr_watch",XC_left_ptr);
    cursors[OB_CURSOR_BUSY] = load_cursor("watch", XC_watch);
    cursors[OB_CURSOR_MOVE] = load_cursor("fleur", XC_fleur);
    cursors[OB_CURSOR_NORTH] = load_cursor("top_side", XC_top_side);
    cursors[OB_CURSOR_NORTHEAST] = load_cursor("top_right_corner",
                                               XC_top_right_corner);
    cursors[OB_CURSOR_EAST] = load_cursor("right_side", XC_right_side);
    cursors[OB_CURSOR_SOUTHEAST] = load_cursor("bottom_right_corner",
                                               XC_bottom_right_corner);
    cursors[OB_CURSOR_SOUTH] = load_cursor("bottom_side", XC_bottom_side);
    cursors[OB_CURSOR_SOUTHWEST] = load_cursor("bottom_left_corner",
                                               XC_bottom_left_corner);
    cursors[OB_CURSOR_WEST] = load_cursor("left_side", XC_left_side);
    cursors[OB_CURSOR_NORTHWEST] = load_cursor("top_left_corner",
                                               XC_top_left_corner);

    prop_startup(); /* get atoms values for the display */
    extensions_query_all(); /* find which extensions are present */

    if (screen_annex()) { /* it will be ours! */
        do {
            ObPrompt *xmlprompt = NULL;

            modkeys_startup(reconfigure);

            /* get the keycodes for keys we use */
            keys[OB_KEY_RETURN] = modkeys_sym_to_code(XK_Return);
            keys[OB_KEY_ESCAPE] = modkeys_sym_to_code(XK_Escape);
            keys[OB_KEY_LEFT] = modkeys_sym_to_code(XK_Left);
            keys[OB_KEY_RIGHT] = modkeys_sym_to_code(XK_Right);
            keys[OB_KEY_UP] = modkeys_sym_to_code(XK_Up);
            keys[OB_KEY_DOWN] = modkeys_sym_to_code(XK_Down);
            keys[OB_KEY_TAB] = modkeys_sym_to_code(XK_Tab);
            keys[OB_KEY_SPACE] = modkeys_sym_to_code(XK_space);
            keys[OB_KEY_HOME] = modkeys_sym_to_code(XK_Home);
            keys[OB_KEY_END] = modkeys_sym_to_code(XK_End);

            {
                ObParseInst *i;
                xmlDocPtr doc;
                xmlNodePtr node;

                /* startup the parsing so everything can register sections
                   of the rc */
                i = parse_startup();

                /* register all the available actions */
                actions_startup(reconfigure);
                /* start up config which sets up with the parser */
                config_startup(i);

                /* parse/load user options */
                if (parse_load_rc(config_file, &doc, &node)) {
                    parse_tree(i, doc, node->xmlChildrenNode);
                    parse_close(doc);
                }
                else {
                    g_message(_("Unable to find a valid config file, using some simple defaults"));
                    config_file = NULL;
                }

                if (config_file) {
                    gchar *p = g_filename_to_utf8(config_file, -1,
                                                  NULL, NULL, NULL);
                    if (p)
                        PROP_SETS(RootWindow(ob_display, ob_screen),
                                  ob_config_file, p);
                    g_free(p);
                }
                else
                    PROP_ERASE(RootWindow(ob_display, ob_screen),
                               ob_config_file);

                /* we're done with parsing now, kill it */
                parse_shutdown(i);
            }

            /* load the theme specified in the rc file */
            {
                RrTheme *theme;
                if ((theme = RrThemeNew(ob_rr_inst, config_theme, TRUE,
                                        config_font_activewindow,
                                        config_font_inactivewindow,
                                        config_font_menutitle,
                                        config_font_menuitem,
                                        config_font_osd)))
                {
                    RrThemeFree(ob_rr_theme);
                    ob_rr_theme = theme;
                }
                if (ob_rr_theme == NULL)
                    ob_exit_with_error(_("Unable to load a theme."));

                PROP_SETS(RootWindow(ob_display, ob_screen),
                          ob_theme, ob_rr_theme->name);
            }

            if (reconfigure) {
                GList *it;

                /* update all existing windows for the new theme */
                for (it = client_list; it; it = g_list_next(it)) {
                    ObClient *c = it->data;
                    frame_adjust_theme(c->frame);
                }
            }
            event_startup(reconfigure);
            /* focus_backup is used for stacking, so this needs to come before
               anything that calls stacking_add */
            sn_startup(reconfigure);
            window_startup(reconfigure);
            focus_startup(reconfigure);
            focus_cycle_startup(reconfigure);
            focus_cycle_indicator_startup(reconfigure);
            focus_cycle_popup_startup(reconfigure);
            screen_startup(reconfigure);
            grab_startup(reconfigure);
            group_startup(reconfigure);
            ping_startup(reconfigure);
            client_startup(reconfigure);
            dock_startup(reconfigure);
            moveresize_startup(reconfigure);
            keyboard_startup(reconfigure);
            mouse_startup(reconfigure);
            menu_frame_startup(reconfigure);
            menu_startup(reconfigure);
            prompt_startup(reconfigure);
            if (!reconfigure) {
                guint32 xid;
                ObWindow *w;

                /* get all the existing windows */
                client_manage_all();
                focus_nothing();

                /* focus what was focused if a wm was already running */
                if (PROP_GET32(RootWindow(ob_display, ob_screen),
                               net_active_window, window, &xid) &&
                    (w = g_hash_table_lookup(window_map, &xid)) &&
                    WINDOW_IS_CLIENT(w))
                {
                    client_focus(WINDOW_AS_CLIENT(w));
                }
            } else {
                GList *it;

                /* redecorate all existing windows */
                for (it = client_list; it; it = g_list_next(it)) {
                    ObClient *c = it->data;

                    /* the new config can change the window's decorations */
                    client_setup_decor_and_functions(c, FALSE);
                    /* redraw the frames */
                    frame_adjust_area(c->frame, TRUE, TRUE, FALSE);
                    /* the decor sizes may have changed, so the windows may
                       end up in new positions */
                    client_reconfigure(c, FALSE);
                }
            }

            reconfigure = FALSE;

            ob_set_state(OB_STATE_RUNNING);

            /* look for parsing errors */
            {
                xmlErrorPtr e = xmlGetLastError();
                if (e) {
                    gchar *m;

                    m = g_strdup_printf(_("One or more XML syntax errors were found while parsing the Openbox configuration files.  See stdout for more information.  The last error seen was in file \"%s\" line %d, with message: %s"), e->file, e->line, e->message);
                    xmlprompt =
                        prompt_show_message(m, _("Openbox Syntax Error"), _("Close"));
                    g_free(m);
                    xmlResetError(e);
                }
            }

			wait_on_socket(reconfigure);
            ob_main_loop_run(ob_main_loop);
            ob_set_state(reconfigure ?
                         OB_STATE_RECONFIGURING : OB_STATE_EXITING);

            if (xmlprompt) {
                prompt_unref(xmlprompt);
                xmlprompt = NULL;
            }

            if (!reconfigure) {
                dock_remove_all();
                client_unmanage_all();
            }

            prompt_shutdown(reconfigure);
            menu_shutdown(reconfigure);
            menu_frame_shutdown(reconfigure);
            mouse_shutdown(reconfigure);
            keyboard_shutdown(reconfigure);
            moveresize_shutdown(reconfigure);
            dock_shutdown(reconfigure);
            client_shutdown(reconfigure);
            ping_shutdown(reconfigure);
            group_shutdown(reconfigure);
            grab_shutdown(reconfigure);
            screen_shutdown(reconfigure);
            focus_cycle_popup_shutdown(reconfigure);
            focus_cycle_indicator_shutdown(reconfigure);
            focus_cycle_shutdown(reconfigure);
            focus_shutdown(reconfigure);
            window_shutdown(reconfigure);
            sn_shutdown(reconfigure);
            event_shutdown(reconfigure);
            config_shutdown();
            actions_shutdown(reconfigure);

            /* Free the key codes for built in keys */
            g_free(keys[OB_KEY_RETURN]);
            g_free(keys[OB_KEY_ESCAPE]);
            g_free(keys[OB_KEY_LEFT]);
            g_free(keys[OB_KEY_RIGHT]);
            g_free(keys[OB_KEY_UP]);
            g_free(keys[OB_KEY_DOWN]);
            g_free(keys[OB_KEY_TAB]);
            g_free(keys[OB_KEY_SPACE]);
            g_free(keys[OB_KEY_HOME]);
            g_free(keys[OB_KEY_END]);

            modkeys_shutdown(reconfigure);
        } while (reconfigure);
    }

    XSync(ob_display, FALSE);

    RrThemeFree(ob_rr_theme);
    RrImageCacheUnref(ob_rr_icons);
    RrInstanceFree(ob_rr_inst);

    session_shutdown(being_replaced);

    XCloseDisplay(ob_display);

    parse_paths_shutdown();

    if (restart) {
        if (restart_path != NULL) {
            gint argcp;
            gchar **argvp;
            GError *err = NULL;

            /* run other window manager */
            if (g_shell_parse_argv(restart_path, &argcp, &argvp, &err)) {
                execvp(argvp[0], argvp);
                g_strfreev(argvp);
            } else {
                g_message(
                    _("Restart failed to execute new executable \"%s\": %s"),
                    restart_path, err->message);
                g_error_free(err);
            }
        }

        /* we remove the session arguments from argv, so put them back,
           also don't restore the session on restart */
        if (ob_sm_save_file != NULL || ob_sm_id != NULL) {
            gchar **nargv;
            gint i, l;

            l = argc + 1 +
                (ob_sm_save_file != NULL ? 2 : 0) +
                (ob_sm_id != NULL ? 2 : 0);
            nargv = g_new0(gchar*, l+1);
            for (i = 0; i < argc; ++i)
                nargv[i] = argv[i];

            if (ob_sm_save_file != NULL) {
                nargv[i++] = g_strdup("--sm-save-file");
                nargv[i++] = ob_sm_save_file;
            }
            if (ob_sm_id != NULL) {
                nargv[i++] = g_strdup("--sm-client-id");
                nargv[i++] = ob_sm_id;
            }
            nargv[i++] = g_strdup("--sm-no-load");
            g_assert(i == l);
            argv = nargv;
        }

        /* re-run me */
        execvp(argv[0], argv); /* try how we were run */
        execlp(argv[0], program_name, (gchar*)NULL); /* last resort */
    }

    /* free stuff passed in from the command line or environment */
    g_free(ob_sm_save_file);
    g_free(ob_sm_id);
    g_free(program_name);

    return exitcode;
}

static void signal_handler(gint signal, gpointer data)
{
    switch (signal) {
    case SIGUSR1:
        ob_debug("Caught signal %d. Restarting.\n", signal);
        ob_restart();
        break;
    case SIGUSR2:
        ob_debug("Caught signal %d. Reconfiguring.\n", signal);
        ob_reconfigure();
        break;
    case SIGCHLD:
        /* reap children */
        while (waitpid(-1, NULL, WNOHANG) > 0);
        break;
    case SIGTTIN:
    case SIGTTOU:
        ob_debug("Caught signal %d. Ignoring.", signal);
        break;
    default:
        ob_debug("Caught signal %d. Exiting.\n", signal);
        /* TERM and INT return a 0 code */
        ob_exit(!(signal == SIGTERM || signal == SIGINT));
    }
}

static void print_version(void)
{
    g_print("Openbox %s\n", PACKAGE_VERSION);
    g_print(_("Copyright (c)"));
    g_print(" 2008        Mikael Magnusson\n");
    g_print(_("Copyright (c)"));
    g_print(" 2003-2006   Dana Jansens\n\n");
    g_print("This program comes with ABSOLUTELY NO WARRANTY.\n");
    g_print("This is free software, and you are welcome to redistribute it\n");
    g_print("under certain conditions. See the file COPYING for details.\n\n");
}

static void print_help(void)
{
    g_print(_("Syntax: openbox [options]\n"));
    g_print(_("\nOptions:\n"));
    g_print(_("  --help              Display this help and exit\n"));
    g_print(_("  --version           Display the version and exit\n"));
    g_print(_("  --replace           Replace the currently running window manager\n"));
    /* TRANSLATORS: if you translate "FILE" here, make sure to keep the "Specify..."
       aligned still, if you have to, make a new line with \n and 22 spaces. It's
       fine to leave it as FILE though. */
    g_print(_("  --config-file FILE  Specify the path to the config file to use\n"));
    g_print(_("  --sm-disable        Disable connection to the session manager\n"));
    g_print(_("\nPassing messages to a running Openbox instance:\n"));
    g_print(_("  --reconfigure       Reload Openbox's configuration\n"));
    g_print(_("  --restart           Restart Openbox\n"));
    g_print(_("  --exit              Exit Openbox\n"));
    g_print(_("\nDebugging options:\n"));
    g_print(_("  --sync              Run in synchronous mode\n"));
    g_print(_("  --debug             Display debugging output\n"));
    g_print(_("  --debug-focus       Display debugging output for focus handling\n"));
    g_print(_("  --debug-xinerama    Split the display into fake xinerama screens\n"));
    g_print(_("\nPlease report bugs at %s\n"), PACKAGE_BUGREPORT);
}

static void remove_args(gint *argc, gchar **argv, gint index, gint num)
{
    gint i;

    for (i = index; i < *argc - num; ++i)
        argv[i] = argv[i+num];
    for (; i < *argc; ++i)
        argv[i] = NULL;
    *argc -= num;
}

static void parse_env(void)
{
    const gchar *id;

    /* unset this so we don't pass it on unknowingly */
    unsetenv("DESKTOP_STARTUP_ID");

    /* this is how gnome-session passes in a session client id */
    id = g_getenv("DESKTOP_AUTOSTART_ID");
    if (id) {
        unsetenv("DESKTOP_AUTOSTART_ID");
        if (ob_sm_id) g_free(ob_sm_id);
        ob_sm_id = g_strdup(id);
        ob_debug_type(OB_DEBUG_SM,
                      "DESKTOP_AUTOSTART_ID %s supercedes --sm-client-id\n",
                      ob_sm_id);
    }
}

static void parse_args(gint *argc, gchar **argv)
{
    gint i;

    for (i = 1; i < *argc; ++i) {
        if (!strcmp(argv[i], "--version")) {
            print_version();
            exit(0);
        }
        else if (!strcmp(argv[i], "--help")) {
            print_help();
            exit(0);
        }
        else if (!strcmp(argv[i], "--g-fatal-warnings")) {
            g_log_set_always_fatal(G_LOG_LEVEL_CRITICAL);
        }
        else if (!strcmp(argv[i], "--replace")) {
            ob_replace_wm = TRUE;
            remove_args(argc, argv, i, 1);
            --i; /* this arg was removed so go back */
        }
        else if (!strcmp(argv[i], "--sync")) {
            xsync = TRUE;
        }
        else if (!strcmp(argv[i], "--debug")) {
            ob_debug_show_output(TRUE);
            ob_debug_enable(OB_DEBUG_SM, TRUE);
            ob_debug_enable(OB_DEBUG_APP_BUGS, TRUE);
        }
        else if (!strcmp(argv[i], "--debug-focus")) {
            ob_debug_show_output(TRUE);
            ob_debug_enable(OB_DEBUG_SM, TRUE);
            ob_debug_enable(OB_DEBUG_APP_BUGS, TRUE);
            ob_debug_enable(OB_DEBUG_FOCUS, TRUE);
        }
        else if (!strcmp(argv[i], "--debug-xinerama")) {
            ob_debug_xinerama = TRUE;
        }
        else if (!strcmp(argv[i], "--reconfigure")) {
            remote_control = 1;
        }
        else if (!strcmp(argv[i], "--restart")) {
            remote_control = 2;
        }
        else if (!strcmp(argv[i], "--exit")) {
            remote_control = 3;
        }
        else if (!strcmp(argv[i], "--config-file")) {
            if (i == *argc - 1) /* no args left */
                g_printerr(_("--config-file requires an argument\n"));
            else {
                /* this will be in the current locale encoding, which is
                   what we want */
                config_file = argv[i+1];
                ++i; /* skip the argument */
                ob_debug("--config-file %s\n", config_file);
            }
        }
        else if (!strcmp(argv[i], "--sm-save-file")) {
            if (i == *argc - 1) /* no args left */
                /* not translated cuz it's sekret */
                g_printerr("--sm-save-file requires an argument\n");
            else {
                ob_sm_save_file = g_strdup(argv[i+1]);
                remove_args(argc, argv, i, 2);
                --i; /* this arg was removed so go back */
                ob_debug_type(OB_DEBUG_SM, "--sm-save-file %s\n",
                              ob_sm_save_file);
            }
        }
        else if (!strcmp(argv[i], "--sm-client-id")) {
            if (i == *argc - 1) /* no args left */
                /* not translated cuz it's sekret */
                g_printerr("--sm-client-id requires an argument\n");
            else {
                ob_sm_id = g_strdup(argv[i+1]);
                remove_args(argc, argv, i, 2);
                --i; /* this arg was removed so go back */
                ob_debug_type(OB_DEBUG_SM, "--sm-client-id %s\n", ob_sm_id);
            }
        }
        else if (!strcmp(argv[i], "--sm-disable")) {
            ob_sm_use = FALSE;
        }
        else if (!strcmp(argv[i], "--sm-no-load")) {
            ob_sm_restore = FALSE;
            remove_args(argc, argv, i, 1);
            --i; /* this arg was removed so go back */
        }
        else {
            /* this is a memleak.. oh well.. heh */
            gchar *err = g_strdup_printf
                (_("Invalid command line argument \"%s\"\n"), argv[i]);
            ob_exit_with_error(err);
        }
    }
}

static Cursor load_cursor(const gchar *name, guint fontval)
{
    Cursor c = None;

#if USE_XCURSOR
    c = XcursorLibraryLoadCursor(ob_display, name);
#endif
    if (c == None)
        c = XCreateFontCursor(ob_display, fontval);
    return c;
}

void ob_exit_with_error(const gchar *msg)
{
    g_message("%s", msg);
    session_shutdown(TRUE);
    exit(EXIT_FAILURE);
}

void ob_restart_other(const gchar *path)
{
    restart_path = g_strdup(path);
    ob_restart();
}

void ob_restart(void)
{
    restart = TRUE;
    ob_exit(0);
}

void ob_reconfigure(void)
{
    reconfigure = TRUE;
    ob_exit(0);
}

void ob_exit(gint code)
{
    exitcode = code;
    ob_main_loop_exit(ob_main_loop);
}

void ob_exit_replace(void)
{
    exitcode = 0;
    being_replaced = TRUE;
    ob_main_loop_exit(ob_main_loop);
}

Cursor ob_cursor(ObCursor cursor)
{
    g_assert(cursor < OB_NUM_CURSORS);
    return cursors[cursor];
}

gboolean ob_keycode_match(KeyCode code, ObKey key)
{
    KeyCode *k;
    
    g_assert(key < OB_NUM_KEYS);
    for (k = keys[key]; *k; ++k)
        if (*k == code) return TRUE;
    return FALSE;
}

ObState ob_state(void)
{
    return state;
}

void ob_set_state(ObState s)
{
    state = s;
}

















//add my func 
/*****************************************************************************
 * Copyright: 
 * File name: 
 * Description: 
 * Author: dandan
 * Version: 0.1
 * Date: 
 * History: 
 * *****************************************************************************/


#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h> 

/* global var*/
int sockfd = 0;
OB_SOCKET obSocket ;
int main_init(int argc, char **argv);
/*
int main(int argc, char **argv){
	start_socket_server(3333);
	socket_xml_exec();
	//main_init(argc,argv);
}
*/
int main_init(int argc, char **argv)
{
       xmlDocPtr doc = NULL,doc2=NULL;       /* document pointer */
	xmlChar *buf;
	int recvBufSize,sendBufSize;
       xmlNodePtr root_node = NULL, node1 = NULL, node2 = NULL,node3 = NULL; /* node pointers */                                                      
       //Creates a new document, a node and set it as a root node
       doc = xmlNewDoc(BAD_CAST "1.0");
       root_node = xmlNewNode(NULL, BAD_CAST "root");
       xmlDocSetRootElement(doc, root_node);    
       //creates a new node, which is "attached" as child node of root_node node. 
       // xmlNewProp() creates attributes, which is "attached" to an node.
       node1=xmlNewChild(root_node, NULL, BAD_CAST "method", BAD_CAST"raise");
       node1 =xmlNewChild(root_node, NULL, BAD_CAST "src",BAD_CAST "remote_dev");
       node1 =xmlNewChild(root_node, NULL, BAD_CAST "id",BAD_CAST "0");
       //xmlNewProp(node1, BAD_CAST "attribute", BAD_CAST "yes"); 
	node1=xmlNewChild(root_node, NULL, BAD_CAST "app",NULL);
	node2 = xmlNewChild(node1,NULL,BAD_CAST "winid",BAD_CAST "111");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "pid",BAD_CAST "222");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "title",BAD_CAST "333");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "cmd",BAD_CAST "444");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "name",BAD_CAST "555");	
	node1 = xmlNewChild(root_node,NULL,BAD_CAST "data",NULL);
	node2 = xmlNewChild(node1,NULL,BAD_CAST "x",BAD_CAST "100");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "y",BAD_CAST "100");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "width",BAD_CAST "200");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "height",BAD_CAST "200");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "extend",BAD_CAST "0");	

       xmlSaveFormatFileEnc(argc > 1 ? argv[1] : "-", doc, "UTF-8", 1); 
	xmlDocDumpFormatMemory(doc,&buf,&recvBufSize,1);
	syslog(LOG_INFO,"char ->%d\n%s",recvBufSize,buf);
       xmlFreeDoc(doc);
	//read from memory
	//doc2 = xmlReadMemory(buf,bufferSize,NULL,NULL,0);
	int type = 0;
		
	start_socket_server(3333);
	socket_xml_exec();
	//syslog(LOG_INFO,"read from char *-->\n");
	//xmlSaveFormatFileEnc(argc > 1 ? argv[1] : "-", doc2, "UTF-8", 1); 
       xmlCleanupParser();
       xmlMemoryDump();      //debug memory for regression tests

       return(0);
}
xmlNodePtr create_xml_from_client(ObClient *c)
{
	char tmp[XML_BUF_SIZE];
	char *p =tmp;	
	//xmlNodePtr app  = xmlNewChild(*headNode,NULL,BAD_CAST("app"),NULL);
	xmlNodePtr app  = xmlNewNode(NULL,BAD_CAST("app"));
	memset(p,0,XML_BUF_SIZE);
	sprintf(tmp,"%d",c->pid);
	xmlNewChild(app,NULL,BAD_CAST "pid",BAD_CAST(tmp));
	xmlNewChild(app,NULL,BAD_CAST "name",BAD_CAST(c->name));
	memset(p,0,XML_BUF_SIZE);
	sprintf(tmp,"%d",c->window);
	xmlNewChild(app,NULL,BAD_CAST "winid",BAD_CAST(tmp));
	xmlNewChild(app,NULL,BAD_CAST "title",BAD_CAST(c->title));
	xmlNewChild(app,NULL,BAD_CAST "cmd",BAD_CAST(c->wm_command));
	memset(p,0,XML_BUF_SIZE);
	sprintf(tmp,"%d",c->desktop);
	xmlNewChild(app,NULL,BAD_CAST "desktop",BAD_CAST(tmp));
	memset(p,0,XML_BUF_SIZE);
	sprintf(tmp,"%d",c->area.x);
	xmlNewChild(app,NULL,BAD_CAST "x",BAD_CAST(tmp));
	memset(p,0,XML_BUF_SIZE);
	sprintf(tmp,"%d",c->area.y);
	xmlNewChild(app,NULL,BAD_CAST "y",BAD_CAST(tmp));
	memset(p,0,XML_BUF_SIZE);
	sprintf(tmp,"%d",c->area.width);
	xmlNewChild(app,NULL,BAD_CAST "width",BAD_CAST(tmp));
	memset(p,0,XML_BUF_SIZE);
	sprintf(tmp,"%d",c->area.height);
	xmlNewChild(app,NULL,BAD_CAST "height",BAD_CAST(tmp));
	memset(p,0,XML_BUF_SIZE);
	sprintf(tmp,"%d",c->above);
	xmlNewChild(app,NULL,BAD_CAST "above",BAD_CAST(tmp));
	memset(p,0,XML_BUF_SIZE);
	sprintf(tmp,"%d",c->below);
	xmlNewChild(app,NULL,BAD_CAST "below",BAD_CAST(tmp));
	memset(p,0,XML_BUF_SIZE);
	sprintf(tmp,"%d",c->type);
	xmlNewChild(app,NULL,BAD_CAST "type",BAD_CAST(tmp));
	memset(p,0,XML_BUF_SIZE);
	sprintf(tmp,"%d",c->max_horz);
	xmlNewChild(app,NULL,BAD_CAST "horz",BAD_CAST(tmp));
	memset(p,0,XML_BUF_SIZE);
	sprintf(tmp,"%d",c->max_vert);
	xmlNewChild(app,NULL,BAD_CAST "vert",BAD_CAST(tmp));
	return app;
	
}

void *get_client_from_app_info(OB_SOCKET *ob)
{
	ObClient *obc = NULL;
	GList *it = NULL;
	guint size = g_list_length(client_list);
	if(size <= 0)
		return NULL;
	for(it=client_list;it;it =  g_list_next(it))
	{
		obc = (ObClient*)it->data;
		////syslog(LOG_INFO,"query client ->%d->%d",ob->appInfo.winid,ob->appInfo.pid);
		if(ob->appInfo.winid == obc->window || ob->appInfo.pid == obc->pid){
			syslog(LOG_INFO,"find client -> %d",obc->window);
			return obc;

		}
	}
	return NULL;
}
int ob_start_app(OB_SOCKET *ob,xmlNodePtr *headNode){
	syslog(LOG_INFO,"ob start app");
	pid_t pid = fork();
	if(pid < 0)
		return -1;
	else if(pid == 0){
		//child pid
		//exec(ob->appInfo.cmd);	
		exit(errno);	
	}
	else
	    return 1;
}
int ob_kill_app(OB_SOCKET *ob,xmlNodePtr *headNode){
	syslog(LOG_INFO,"ob kill app");
    ObClient *it = get_client_from_app_info(ob);
	if(it != NULL)
	{
		char cmd[40];
		sprintf(cmd,"kill %d",ob->appInfo.pid);	
		system(cmd);
        return 1;
	}
	else
		return -1;
}
int ob_exit_app(OB_SOCKET *ob,xmlNodePtr *headNode){
	syslog(LOG_INFO,"ob exit app");
    ObClient *it = get_client_from_app_info(ob);
	if(it != NULL)
	{
		char cmd[40];//let process exit auto
		sprintf(cmd,"kill -3 %d",ob->appInfo.pid);	
		system(cmd);
        return 1;
	}
	else
		return -1;
}
int ob_set_full_app(OB_SOCKET *ob,xmlNodePtr *headNode){
    ObClient *it = get_client_from_app_info(ob);
	syslog(LOG_INFO,"ob full app");
	if(it != NULL)
	{
		syslog(LOG_INFO,"client state -> %d",it->fullscreen);
		client_fullscreen(it,!it->fullscreen);    
		client_focus(it);
		return 1;
	}
	else
	{
		return -1;
	}
}
int ob_set_max_app(OB_SOCKET *ob,xmlNodePtr *headNode){
    ObClient *it = get_client_from_app_info(ob);
	syslog(LOG_INFO,"ob max app");
	if(it != NULL){
		if(it->max_vert && it->max_horz)
		{
			client_maximize(it,FALSE,0); 
		}
		else
		{
			client_maximize(it,TRUE,0); 
		}
		
		xmlNewChild(*headNode,NULL,BAD_CAST("error"),BAD_CAST "OK"); 
		xmlNodePtr app = create_xml_from_client(it);
		xmlAddChild(*headNode,app);
		return 1;
	}
	else
	{
		xmlNewChild(*headNode,NULL,BAD_CAST("error"),BAD_CAST "can find window id"); 
	
		return -1;
	}
//		client_activate(it,TRUE,TRUE,TRUE,TRUE,TRUE);
}
int ob_set_min_app(OB_SOCKET *ob,xmlNodePtr *headNode){
    ObClient *it = get_client_from_app_info(ob);
	syslog(LOG_INFO,"ob min app");
	if(it != NULL)
	{
		client_hide(it);    

		return 1;
	}
	else
		return -1;
}
int ob_set_layer_app(OB_SOCKET *ob,int layer,xmlNodePtr *headNode){
    ObClient *it = get_client_from_app_info(ob);
	syslog(LOG_INFO,"ob layer app->%d",layer);

	if(it != NULL)
	{
		client_set_layer(it,layer);
		return 1;
	}
	else
		return -1;
}
int ob_send_to_extend(OB_SOCKET *ob,xmlNodePtr *headNode)
{
	syslog(LOG_INFO,"method send to extend");
    ObClient *it = get_client_from_app_info(ob);
	if(it != NULL)
	{
		//client_set_layer(it,layer);
		client_move(it,1300,10);
		client_fullscreen(it,TRUE);
		return 1;
	}
	else
		return -1;
}
int ob_send_to_main(OB_SOCKET *ob,xmlNodePtr *headNode)
{
	syslog(LOG_INFO,"method send to main");
    ObClient *it = get_client_from_app_info(ob);
	if(it != NULL)
	{
		client_move(it,10,10);
		client_fullscreen(it,TRUE);
		return 1;
	}
	else
		return -1;
}
int ob_raise(OB_SOCKET *ob,xmlNodePtr *headNode)
{
	syslog(LOG_INFO,"method raise");
    ObClient *it = get_client_from_app_info(ob);
	if(it != NULL)
	{
		//client_move(it,10,10);
		client_activate(it,TRUE,TRUE,TRUE,TRUE,TRUE);
		//client_fullscreen(it,TRUE);
		return 1;
	}
	else
		return -1;
}
int ob_focus(OB_SOCKET *ob,xmlNodePtr *headNode)
{
	syslog(LOG_INFO,"method focus");
    ObClient *it = get_client_from_app_info(ob);
	if(it != NULL)
	{
		client_activate(it,TRUE,TRUE,TRUE,TRUE,TRUE);
		return 1;
	}
	else
		return -1;
}
int ob_show_desktop(OB_SOCKET *ob,xmlNodePtr *headNode)
{
	syslog(LOG_INFO,"method send to main");
    ObClient *it = get_client_from_app_info(ob);
	if(it != NULL)
	{
	//	client_move(it,10,10);
	//	client_fullscreen(it,TRUE);
		return 1;
	}
	else
		return -1;
}
int ob_get_list_app(OB_SOCKET *ob,xmlNodePtr *headNode)
{
	syslog(LOG_INFO,"get app list");
	Window *windows,*win_it;
	ObClient *c;
	GList *it;
	xmlNodePtr app = NULL,node1;
	char tmp[XML_BUF_SIZE];
	char *p = tmp;
	int counter = 1;
	guint size = g_list_length(client_list);
	//demo test
	if(size > 0)
	{
		windows =g_new(Window,size);
		win_it = windows;
		
		for(it=client_list;it;it =  g_list_next(it),++win_it)
		{
			*win_it = ((ObClient*)it->data)->window;
			c= (ObClient*)it->data;
			//syslog(LOG_INFO,"client ->%d->%d->%d->%d->%d->%d->%d->%d->%d->%d->%s->%s->%s",c->obwin.type,c->window,c->desktop,c->area.x,c->area.y,c->area.width,c->area.height,c->root_pos.x,c->root_pos.y,c->layer,c->title,c->wm_command,c->name);
			 app = create_xml_from_client(c);
			xmlAddChild(*headNode,app);
		//	xmlAddChild(*headNode,app);
		//	app  = xmlNewNode(NULL,BAD_CAST "app");
		//	node1 = xmlNewText(BAD_CAST"other way to create content"); 
		//	xmlAddChild(app,node1);
		//	xmlAddChild(*headNode,app);
			continue;
			//not exec
			return 1;
			memset(p,0,XML_BUF_SIZE);
			sprintf(tmp,"app-%d",counter++);
			app  = xmlNewChild(*headNode,NULL,BAD_CAST("app"),NULL);
			memset(p,0,XML_BUF_SIZE);
			sprintf(tmp,"%d",c->pid);
			xmlNewChild(app,NULL,BAD_CAST "pid",BAD_CAST(tmp));
			xmlNewChild(app,NULL,BAD_CAST "name",BAD_CAST(c->name));
			memset(p,0,XML_BUF_SIZE);
			sprintf(tmp,"%d",c->window);
			xmlNewChild(app,NULL,BAD_CAST "winid",BAD_CAST(tmp));
			xmlNewChild(app,NULL,BAD_CAST "title",BAD_CAST(c->title));
			xmlNewChild(app,NULL,BAD_CAST "cmd",BAD_CAST(c->wm_command));
			memset(p,0,XML_BUF_SIZE);
			sprintf(tmp,"%d",c->desktop);
			xmlNewChild(app,NULL,BAD_CAST "desktop",BAD_CAST(tmp));
			memset(p,0,XML_BUF_SIZE);
			sprintf(tmp,"%d",c->area.x);
			xmlNewChild(app,NULL,BAD_CAST "x",BAD_CAST(tmp));
			memset(p,0,XML_BUF_SIZE);
			sprintf(tmp,"%d",c->area.y);
			xmlNewChild(app,NULL,BAD_CAST "y",BAD_CAST(tmp));
			memset(p,0,XML_BUF_SIZE);
			sprintf(tmp,"%d",c->area.width);
			xmlNewChild(app,NULL,BAD_CAST "width",BAD_CAST(tmp));
			memset(p,0,XML_BUF_SIZE);
			sprintf(tmp,"%d",c->area.height);
			xmlNewChild(app,NULL,BAD_CAST "height",BAD_CAST(tmp));
			memset(p,0,XML_BUF_SIZE);
			sprintf(tmp,"%d",c->above);
			xmlNewChild(app,NULL,BAD_CAST "above",BAD_CAST(tmp));
			memset(p,0,XML_BUF_SIZE);
			sprintf(tmp,"%d",c->below);
			xmlNewChild(app,NULL,BAD_CAST "below",BAD_CAST(tmp));
			memset(p,0,XML_BUF_SIZE);
			sprintf(tmp,"%d",c->type);
			xmlNewChild(app,NULL,BAD_CAST "type",BAD_CAST(tmp));
		
		}
	}
	else
		windows = NULL;
	if(windows)
		free(windows);

	return 1;	
}
int ob_get_app_state(OB_SOCKET *ob,xmlNodePtr *headNode){
    ObClient *it = get_client_from_app_info(ob);
	syslog(LOG_INFO,"ob  app state");
	char tmp[XML_BUF_SIZE];
	char *p = tmp;
	if(it != NULL)
	{
		xmlNodePtr app  = xmlNewChild(*headNode,NULL,BAD_CAST "app",NULL);
		memset(p,0,XML_BUF_SIZE);
        sprintf(tmp,"%d",it->pid);
        xmlNewChild(app,NULL,BAD_CAST "pid",BAD_CAST(tmp));
        xmlNewChild(app,NULL,BAD_CAST "name",BAD_CAST(it->name));
        memset(p,0,XML_BUF_SIZE);
        sprintf(tmp,"%d",it->window);
        xmlNewChild(app,NULL,BAD_CAST "winid",BAD_CAST(tmp));
        xmlNewChild(app,NULL,BAD_CAST "title",BAD_CAST(it->title));           
		xmlNewChild(app,NULL,BAD_CAST "cmd",BAD_CAST(it->wm_command));
			memset(p,0,XML_BUF_SIZE);
			sprintf(tmp,"%d",it->desktop);
			xmlNewChild(app,NULL,BAD_CAST "desktop",BAD_CAST(tmp));
			memset(p,0,XML_BUF_SIZE);
			sprintf(tmp,"%d",it->area.x);
			xmlNewChild(app,NULL,BAD_CAST "x",BAD_CAST(tmp));
			memset(p,0,XML_BUF_SIZE);
			sprintf(tmp,"%d",it->area.y);
			xmlNewChild(app,NULL,BAD_CAST "y",BAD_CAST(tmp));
			memset(p,0,XML_BUF_SIZE);
			sprintf(tmp,"%d",it->area.width);
			xmlNewChild(app,NULL,BAD_CAST "width",BAD_CAST(tmp));
			memset(p,0,XML_BUF_SIZE);
			sprintf(tmp,"%d",it->area.height);
			xmlNewChild(app,NULL,BAD_CAST "height",BAD_CAST(tmp));
			memset(p,0,XML_BUF_SIZE);
			sprintf(tmp,"%d",it->above);
			xmlNewChild(app,NULL,BAD_CAST "above",BAD_CAST(tmp));
			memset(p,0,XML_BUF_SIZE);
			sprintf(tmp,"%d",it->below);
			xmlNewChild(app,NULL,BAD_CAST "below",BAD_CAST(tmp));
			memset(p,0,XML_BUF_SIZE);
			sprintf(tmp,"%d",it->type);
			xmlNewChild(app,NULL,BAD_CAST "type",BAD_CAST(tmp));
		return 1;
	}
	else
		return -1;
}
int ob_socket_exit(OB_SOCKET *ob,xmlNodePtr *headNode){
	syslog(LOG_INFO,"ob exit");
	ob_exit(0);
	return 1;
}
int ob_socket_restart(OB_SOCKET *ob,xmlNodePtr *headNode){
	
	syslog(LOG_INFO,"ob restart");
	ob_restart();
    return 1;
}
int ob_socket_refresh(OB_SOCKET *ob,xmlNodePtr *headNode){
	syslog(LOG_INFO,"ob refresh");
        return 1;
}
int ob_resize(OB_SOCKET *ob,xmlNodePtr *headNode){
	syslog(LOG_INFO,"ob resize app");
    ObClient *it = get_client_from_app_info(ob);
	if(it != NULL)
	{
		client_resize(it,ob->appData.width,ob->appData.height);    
		return 1;
	}
	else
		return -1;
}
int ob_move(OB_SOCKET *ob,xmlNodePtr *headNode){
    ObClient *it = get_client_from_app_info(ob);
	syslog(LOG_INFO,"ob move app");
	if(it != NULL)
	{
		client_move(it,ob->appData.x,ob->appData.y);    
		return 1;
	}
	else
		return -1;
}
int ob_resize_move(OB_SOCKET *ob,xmlNodePtr *headNode){
	syslog(LOG_INFO,"ob resize and move app");
    ObClient *it = get_client_from_app_info(ob);
	if(it != NULL)
	{
		client_move_resize(it,ob->appData.x,ob->appData.y,ob->appData.width,ob->appData.height);    
		client_calc_layer(it);
		//client_focus(it);
		return 1;
	}
	else
		return -1;
}
int ob_get_system(OB_SOCKET *ob,xmlNodePtr *headNode)
{
	return 1;
}
void trint_element_names(xmlNode * a_node)
{
    xmlNode *cur_node = NULL;
	return ;
}
/*

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            syslog(LOG_INFO,"node->  %d\t%s\t-->%s\n",cur_node->type, cur_node->name,xmlNodeGetContent(cur_node));
        }
        print_element_names(cur_node->children);
    }
}
*/
static xmlXPathObjectPtr
getXPathObjectPtr(xmlDocPtr doc, xmlChar* xpath_exp) {
        xmlXPathObjectPtr result;
        xmlXPathContextPtr context;

        context = xmlXPathNewContext(doc);
        result = xmlXPathEvalExpression((const xmlChar*)xpath_exp, context);
        xmlXPathFreeContext(context);

        if(NULL == result) {
                fprintf(stderr, "eval expression error!\n");
                return NULL;
        }   

        if(xmlXPathNodeSetIsEmpty(result->nodesetval)) {
                fprintf(stderr, "empty node set!\n");
                xmlXPathFreeObject(result);
                return NULL;
        }   
        return result;
}
int query_value_from_xpath(xmlDoc *doc,char *xpath,char **buf,int bufferSize)
{
	memset((void *)(*buf),0,bufferSize);
//	printf("query value");
	xmlXPathObjectPtr xpath_obj = NULL; 
	xmlNodeSetPtr nodeset = NULL;
	xpath_obj = getXPathObjectPtr(doc,BAD_CAST xpath);
     	if(NULL != xpath_obj) {
        	nodeset = xpath_obj->nodesetval;
                int i = 0;
                for(i = 0; i < nodeset->nodeNr; i ++) {
        //                uri = xmlGetProp(nodeset->nodeTab[i],(const xmlChar*)"href");
                        //sprintf(*buf,"link address:%s->%s->%s\n",uri,nodeset->nodeTab[i]->name,
                        //sprintf(*buf,"%s%s",*buf,xmlNodeGetContent(nodeset->nodeTab[i]));
			strcat(*buf,(const char *)xmlNodeGetContent(nodeset->nodeTab[i]));
                        //xmlFree(uri);
                }   
                xmlXPathFreeObject(xpath_obj);
		return 1;
        }   
	else
		return -1;
}
int parse_xml_from_buf(char *buf,int len,OB_SOCKET *ob){
	char fuck[XML_BUF_SIZE];
	char *tmp = (char *)fuck;
	
//	syslog(LOG_INFO,"parese xml from buf ->%d->%s",len,buf);
	memset((void *)ob,0,sizeof(OB_SOCKET));
	xmlDoc  *doc = xmlReadMemory((xmlChar *)buf,len,NULL,NULL,0);
	if(doc == NULL){
		syslog(LOG_INFO,"buf can't conver to xml\n");
		return -1;
	}
	//syslog(LOG_INFO,"app comtrol data +++++++++++++++++++++\n");
	query_value_from_xpath(doc,"//method",&tmp,XML_BUF_SIZE);
	//syslog(LOG_INFO,"xpath method -> %s\n",tmp);
	ob->method = atoi(tmp);
	query_value_from_xpath(doc,"//src",&tmp,XML_BUF_SIZE);
	//syslog(LOG_INFO,"xpath src    -> %s\n",tmp);
	strcat(ob->src,tmp); 
	query_value_from_xpath(doc,"//id",&tmp,XML_BUF_SIZE);
	//syslog(LOG_INFO,"xpath id     -> %s\n",tmp);
	ob->id = atoi(tmp);
	query_value_from_xpath(doc,"//winid",&tmp,XML_BUF_SIZE);
	//syslog(LOG_INFO,"app basic data +++++++++++++++++++++\n");
	//syslog(LOG_INFO,"xpath winid  -> %s\n",tmp);
	ob->appInfo.winid = atoi(tmp);
	query_value_from_xpath(doc,"//pid",&tmp,XML_BUF_SIZE);
	//syslog(LOG_INFO,"xpath pid    -> %s\n",tmp);
	ob->appInfo.pid = atoi(tmp);
	query_value_from_xpath(doc,"//title",&tmp,XML_BUF_SIZE);
	//syslog(LOG_INFO,"xpath title  -> %s\n",tmp);
	strcat(ob->appInfo.title,tmp);
	query_value_from_xpath(doc,"//cmd",&tmp,XML_BUF_SIZE);
	//syslog(LOG_INFO,"xpath cmd    -> %s\n",tmp);
	ob->appInfo.pid = atoi(tmp);
	query_value_from_xpath(doc,"//name",&tmp,XML_BUF_SIZE);
	//syslog(LOG_INFO,"xpath name   -> %s\n",tmp);
	ob->appInfo.pid = atoi(tmp);
	//syslog(LOG_INFO,"app extra data +++++++++++++++++++++\n");
	query_value_from_xpath(doc,"//x",&tmp,XML_BUF_SIZE);
	//syslog(LOG_INFO,"xpath x      -> %s\n",tmp);
	ob->appData.x = atoi(tmp);
	query_value_from_xpath(doc,"//y",&tmp,XML_BUF_SIZE);
	//syslog(LOG_INFO,"xpath y      -> %s\n",tmp);
	ob->appData.y = atoi(tmp);
	query_value_from_xpath(doc,"//width",&tmp,XML_BUF_SIZE);
	//syslog(LOG_INFO,"xpath width  -> %s\n",tmp);
	ob->appData.width = atoi(tmp);
	query_value_from_xpath(doc,"//height",&tmp,XML_BUF_SIZE);
	//syslog(LOG_INFO,"xpath height -> %s\n",tmp);
	ob->appData.height = atoi(tmp);
	query_value_from_xpath(doc,"//extend",&tmp,XML_BUF_SIZE);
	//syslog(LOG_INFO,"xpath extend -> %s\n",tmp);
	ob->appData.extend = atoi(tmp);
	//free docmemery
	xmlFreeDoc(doc); 
	xmlCleanupParser();
	return 1;
}
int exec_socket_cmd(OB_SOCKET *ob,char **ack,int *ack_len,int ackBufSize)
{
	xmlNodePtr dataNode  = xmlNewNode(NULL,BAD_CAST"data");
	int state = 0;
	int len = 0;
	syslog(LOG_INFO,"msg type ->%d",ob->method);
	switch(ob->method)
	{
		case OB_START_APP :
			//start app
			state = ob_start_app(ob,&dataNode);
			break;
		case OB_KILL_APP :
			//kill app
			state = ob_kill_app(ob,&dataNode);
			break;
		case OB_EXIT_APP :
			//exit
			state = ob_exit_app(ob,&dataNode);
			break;
		case OB_SET_FULLSCREEN_APP:
			state = ob_set_full_app(ob,&dataNode);
			break;
		case OB_SET_MAX_APP :
			state = ob_set_max_app(ob,&dataNode);
			break;
		case OB_SET_MIN_APP :
			state = ob_set_min_app(ob,&dataNode);
			break;
		case OB_SET_BOTTOM_APP :
			state = ob_set_layer_app(ob,-1,&dataNode);
			break;
		case OB_SET_TOP_APP :
			state = ob_set_layer_app(ob,1,&dataNode);
			break;
		case OB_SET_NORMAL_APP :
			state = ob_set_layer_app(ob,0,&dataNode);
			break;
		case OB_GET_APPS_LIST :
			state = ob_get_list_app(ob,&dataNode);
			break;
		case OB_GET_APP_STATE :
			state = ob_get_app_state(ob,&dataNode);
			break;
		case OB_EXIT :
			ob_exit(0);
			break;
		case OB_RESTART :
			ob_restart();
			break;
		case OB_REFRESH :
			ob_reconfigure();
			break;
		case OB_RESIZE_APP :
			state = ob_resize(ob,&dataNode);	
			break;
		case OB_MOVE_APP :
			state = ob_move(ob,&dataNode);
			break;
		case OB_RESIZE_MOVE_APP :
			state = ob_resize_move(ob,&dataNode);
			break;
		case OB_SEND_TO_EXTEND_APP :
			state = ob_send_to_extend(ob,&dataNode);
			break;
		case OB_SEND_TO_MAIN_APP :
			state = ob_send_to_main(ob,&dataNode);
			break;
		case OB_GET_SYSTEM :
			state = ob_get_system(ob,&dataNode);
			break;
		case OB_SHOW_DESKTOP :
			state = ob_get_system(ob,&dataNode);
			break;
		case OB_RAISE_APP :
			state = ob_raise(ob,&dataNode);
			break;
		case OB_FOCUS_APP :
			state = ob_focus(ob,&dataNode);
			break;
		default :
			syslog(LOG_INFO,"no defined method");
			break;
	}
	//syslog(LOG_INFO,"xml exec deal ok");
	char tmp[20];
	xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
   	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST "root");
   	xmlDocSetRootElement(doc, root_node); 
	memset(tmp,0,20);
	sprintf(tmp,"%d",ob->id);
	xmlNewChild(root_node, NULL, BAD_CAST "id",BAD_CAST tmp); 
	memset(tmp,0,20);
	if(state >0)
		strcpy(tmp,"OK");
	else
		strcpy(tmp,"ERROR");
	xmlNewChild(root_node, NULL, BAD_CAST "state",BAD_CAST tmp); 
		xmlAddChild(root_node,dataNode); 
	//syslog(LOG_INFO,"xml exec deal ok");
	
	xmlDocDumpFormatMemory(doc,(xmlChar **)ack,&len,1);
	*ack_len = len;
	xmlFreeDoc(doc); 
	xmlCleanupParser();

	//syslog(LOG_INFO,"xml result ->%d--> %s",*ack_len,*ack);	
}
int  start_socket_server(int port)
{
        socklen_t len;
        socklen_t src_len;
        struct sockaddr_in servaddr, cliaddr;
        sockfd = socket(AF_INET, SOCK_DGRAM, 0); /* create a socket */
		if(sockfd == -1){
			syslog(LOG_INFO,"socket bind error");
		}
			syslog(LOG_INFO,"socket id->%d",sockfd);
        /* init s//ervaddr */
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(port);
		syslog(LOG_INFO,"server ->%d",port);
        /* bind address and port to socket */
        if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
        {
            perror("bind error");
		return -1;
		}
	syslog(LOG_INFO,"bind socket ok-->%d\n",sockfd);
}
int  add_data_to_xml_node(xmlNode *node,char *key,char *value)
{
	//add child data
	if(node == NULL)
	{
		syslog(LOG_INFO,"xml node can't be null\n");
		return -1;
	}
	if(xmlNewChild(node,NULL,BAD_CAST key ,BAD_CAST value) < 0)
	{
		syslog(LOG_INFO,"can,t add -> %s -->%s\n",key,value);
		return -1;
	}
	return 1;
}
int  socket_xml_exec(void)
{
	static char rev[SOCKET_BUF_SIZE];
	static char send[SOCKET_BUF_SIZE];
	char *sendPtr = send;
	int revBufSize = 0;
	int sendBufSize = 0,sendResult = 0;
	int state = 0;
	static int counter = 0;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t src_len;
	
	memset(rev,0,SOCKET_BUF_SIZE);
	memset(send,0,SOCKET_BUF_SIZE);
	memset(&cliaddr,0,sizeof(struct sockaddr_in));
	syslog(LOG_INFO,"wait for net socket->%d\n",counter++);
	revBufSize = recvfrom(sockfd, rev, SOCKET_BUF_SIZE, 0, (struct sockaddr *)&cliaddr, &src_len);
	if(revBufSize < 0)
        {
		syslog(LOG_INFO,"receive error!!\n");
		return -1;
        }
	
	syslog(LOG_INFO,"receive data ->%d\n",revBufSize);
	if(parse_xml_from_buf(rev,revBufSize,&obSocket) < 0){
		syslog(LOG_INFO,"parse buf to xml error");
		return -1;
	}
	else{
		syslog(LOG_INFO,"parse buf to xml ok");
	}
	return;
	exec_socket_cmd(&obSocket,&sendPtr,&sendBufSize,SOCKET_BUF_SIZE);
	sendResult=sendto(sockfd, sendPtr, sendBufSize, 0, (struct sockaddr *)&cliaddr, sizeof(struct sockaddr));	
	//syslog(LOG_INFO,"ack buf size ->%d->%d->%s",sendResult,sendBufSize,sendPtr);
	if(revBufSize < 0)
        {
		syslog(LOG_INFO,"send ack data error!\n");
		return -1;
        }
	else if(sendResult != sendBufSize){
		syslog(LOG_INFO,"send ack data lost some -> %d - %d = %d\n",sendBufSize,sendResult, sendBufSize-sendResult);
		return -1;
	}
	return 1;
}

void wait_on_socket(gboolean reconfigure){
	if(reconfigure)
		close(sockfd);
	start_socket_server(SERV_PORT);

}	
