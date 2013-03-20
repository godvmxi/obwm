/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-

   event.c for the Openbox window manager
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

#include "event.h"
#include "debug.h"
#include "window.h"
#include "openbox.h"
#include "dock.h"
#include "actions.h"
#include "client.h"
#include "xerror.h"
#include "prop.h"
#include "config.h"
#include "screen.h"
#include "frame.h"
#include "grab.h"
#include "menu.h"
#include "prompt.h"
#include "menuframe.h"
#include "keyboard.h"
#include "modkeys.h"
#include "mouse.h"
#include "mainloop.h"
#include "focus.h"
#include "focus_cycle.h"
#include "moveresize.h"
#include "group.h"
#include "stacking.h"
#include "extensions.h"
#include "translate.h"
#include "ping.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <glib.h>

#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif
#ifdef HAVE_SIGNAL_H
#  include <signal.h>
#endif
#ifdef HAVE_UNISTD_H
#  include <unistd.h> /* for usleep() */
#endif
#ifdef XKB
#  include <X11/XKBlib.h>
#endif

#ifdef USE_SM
#include <X11/ICE/ICElib.h>
#endif

typedef struct
{
    gboolean ignored;
} ObEventData;

typedef struct
{
    ObClient *client;
    Time time;
    gulong serial;
} ObFocusDelayData;

typedef struct
{
    gulong start; /* inclusive */
    gulong end;   /* inclusive */
} ObSerialRange;

static void event_process(const XEvent *e, gpointer data);
static void event_handle_root(XEvent *e);
static gboolean event_handle_menu_keyboard(XEvent *e);
static gboolean event_handle_menu(XEvent *e);
static gboolean event_handle_prompt(ObPrompt *p, XEvent *e);
static void event_handle_dock(ObDock *s, XEvent *e);
static void event_handle_dockapp(ObDockApp *app, XEvent *e);
static void event_handle_client(ObClient *c, XEvent *e);
static void event_handle_user_input(ObClient *client, XEvent *e);
static gboolean is_enter_focus_event_ignored(gulong serial);
static void event_ignore_enter_range(gulong start, gulong end);

static void focus_delay_dest(gpointer data);
static gboolean focus_delay_cmp(gconstpointer d1, gconstpointer d2);
static gboolean focus_delay_func(gpointer data);
static void focus_delay_client_dest(ObClient *client, gpointer data);

Time event_curtime = CurrentTime;
Time event_last_user_time = CurrentTime;
/*! The serial of the current X event */

static gulong event_curserial;
static gboolean focus_left_screen = FALSE;
/*! A list of ObSerialRanges which are to be ignored for mouse enter events */
static GSList *ignore_serials = NULL;

#ifdef USE_SM
static void ice_handler(gint fd, gpointer conn)
{
    Bool b;
    IceProcessMessages(conn, NULL, &b);
}

static void ice_watch(IceConn conn, IcePointer data, Bool opening,
                      IcePointer *watch_data)
{
    static gint fd = -1;

    if (opening) {
        fd = IceConnectionNumber(conn);
        ob_main_loop_fd_add(ob_main_loop, fd, ice_handler, conn, NULL);
    } else {
        ob_main_loop_fd_remove(ob_main_loop, fd);
        fd = -1;
    }
}
#endif

void event_startup(gboolean reconfig)
{
    if (reconfig) return;

    ob_main_loop_x_add(ob_main_loop, event_process, NULL, NULL);

#ifdef USE_SM
    IceAddConnectionWatch(ice_watch, NULL);
#endif

    client_add_destroy_notify(focus_delay_client_dest, NULL);
}

void event_shutdown(gboolean reconfig)
{
    if (reconfig) return;

#ifdef USE_SM
    IceRemoveConnectionWatch(ice_watch, NULL);
#endif

    client_remove_destroy_notify(focus_delay_client_dest);
}

static Window event_get_window(XEvent *e)
{
    Window window;

    /* pick a window */
    switch (e->type) {
    case SelectionClear:
        window = RootWindow(ob_display, ob_screen);
        break;
    case MapRequest:
        window = e->xmap.window;
        break;
    case UnmapNotify:
        window = e->xunmap.window;
        break;
    case DestroyNotify:
        window = e->xdestroywindow.window;
        break;
    case ConfigureRequest:
        window = e->xconfigurerequest.window;
        break;
    case ConfigureNotify:
        window = e->xconfigure.window;
        break;
    default:
#ifdef XKB
        if (extensions_xkb && e->type == extensions_xkb_event_basep) {
            switch (((XkbAnyEvent*)e)->xkb_type) {
            case XkbBellNotify:
                window = ((XkbBellNotifyEvent*)e)->window;
            default:
                window = None;
            }
        } else
#endif
#ifdef SYNC
        if (extensions_sync &&
            e->type == extensions_sync_event_basep + XSyncAlarmNotify)
        {
            window = None;
        } else
#endif
            window = e->xany.window;
    }
    return window;
}

static void event_set_curtime(XEvent *e)
{
    Time t = CurrentTime;

    /* grab the lasttime and hack up the state */
    switch (e->type) {
    case ButtonPress:
    case ButtonRelease:
        t = e->xbutton.time;
        break;
    case KeyPress:
        t = e->xkey.time;
        break;
    case KeyRelease:
        t = e->xkey.time;
        break;
    case MotionNotify:
        t = e->xmotion.time;
        break;
    case PropertyNotify:
        t = e->xproperty.time;
        break;
    case EnterNotify:
    case LeaveNotify:
        t = e->xcrossing.time;
        break;
    default:
#ifdef SYNC
        if (extensions_sync &&
            e->type == extensions_sync_event_basep + XSyncAlarmNotify)
        {
            t = ((XSyncAlarmNotifyEvent*)e)->time;
        }
#endif
        /* if more event types are anticipated, get their timestamp
           explicitly */
        break;
    }

    /* watch that if we get an event earlier than the last specified user_time,
       which can happen if the clock goes backwards, we erase the last
       specified user_time */
    if (t && event_last_user_time && event_time_after(event_last_user_time, t))
        event_last_user_time = CurrentTime;

    event_curtime = t;
}

static void event_hack_mods(XEvent *e)
{
#ifdef XKB
    XkbStateRec xkb_state;
#endif

    switch (e->type) {
    case ButtonPress:
    case ButtonRelease:
        e->xbutton.state = modkeys_only_modifier_masks(e->xbutton.state);
        break;
    case KeyPress:
        e->xkey.state = modkeys_only_modifier_masks(e->xkey.state);
        break;
    case KeyRelease:
#ifdef XKB
        /* If XKB is present, then the modifiers are all strange from its
           magic.  Our X core protocol stuff won't work, so we use this to
           find what the modifier state is instead. */
        if (XkbGetState(ob_display, XkbUseCoreKbd, &xkb_state) == Success)
            e->xkey.state =
                modkeys_only_modifier_masks(xkb_state.compat_state);
        else
#endif
        {
            e->xkey.state = modkeys_only_modifier_masks(e->xkey.state);
            /* remove from the state the mask of the modifier key being
               released, if it is a modifier key being released that is */
            e->xkey.state &= ~modkeys_keycode_to_mask(e->xkey.keycode);
        }
        break;
    case MotionNotify:
        e->xmotion.state = modkeys_only_modifier_masks(e->xmotion.state);
        /* compress events */
        {
            XEvent ce;
            while (XCheckTypedWindowEvent(ob_display, e->xmotion.window,
                                          e->type, &ce)) {
                e->xmotion.x = ce.xmotion.x;
                e->xmotion.y = ce.xmotion.y;
                e->xmotion.x_root = ce.xmotion.x_root;
                e->xmotion.y_root = ce.xmotion.y_root;
            }
        }
        break;
    }
}

static gboolean wanted_focusevent(XEvent *e, gboolean in_client_only)
{
    gint mode = e->xfocus.mode;
    gint detail = e->xfocus.detail;
    Window win = e->xany.window;

    if (e->type == FocusIn) {
        /* These are ones we never want.. */

        /* This means focus was given by a keyboard/mouse grab. */
        if (mode == NotifyGrab)
            return FALSE;
        /* This means focus was given back from a keyboard/mouse grab. */
        if (mode == NotifyUngrab)
            return FALSE;

        /* These are the ones we want.. */

        if (win == RootWindow(ob_display, ob_screen)) {
            /* If looking for a focus in on a client, then always return
               FALSE for focus in's to the root window */
            if (in_client_only)
                return FALSE;
            /* This means focus reverted off of a client */
            else if (detail == NotifyPointerRoot ||
                     detail == NotifyDetailNone ||
                     detail == NotifyInferior ||
                     /* This means focus got here from another screen */
                     detail == NotifyNonlinear)
                return TRUE;
            else
                return FALSE;
        }

        /* It was on a client, was it a valid one?
           It's possible to get a FocusIn event for a client that was managed
           but has disappeared.
        */
        if (in_client_only) {
            ObWindow *w = g_hash_table_lookup(window_map, &e->xfocus.window);
            if (!w || !WINDOW_IS_CLIENT(w))
                return FALSE;
        }
        else {
            /* This means focus reverted to parent from the client (this
               happens often during iconify animation) */
            if (detail == NotifyInferior)
                return TRUE;
        }

        /* This means focus moved from the root window to a client */
        if (detail == NotifyVirtual)
            return TRUE;
        /* This means focus moved from one client to another */
        if (detail == NotifyNonlinearVirtual)
            return TRUE;

        /* Otherwise.. */
        return FALSE;
    } else {
        g_assert(e->type == FocusOut);

        /* These are ones we never want.. */

        /* This means focus was taken by a keyboard/mouse grab. */
        if (mode == NotifyGrab)
            return FALSE;
        /* This means focus was grabbed on a window and it was released. */
        if (mode == NotifyUngrab)
            return FALSE;

        /* Focus left the root window revertedto state */
        if (win == RootWindow(ob_display, ob_screen))
            return FALSE;

        /* These are the ones we want.. */

        /* This means focus moved from a client to the root window */
        if (detail == NotifyVirtual)
            return TRUE;
        /* This means focus moved from one client to another */
        if (detail == NotifyNonlinearVirtual)
            return TRUE;

        /* Otherwise.. */
        return FALSE;
    }
}

static Bool event_look_for_focusin(Display *d, XEvent *e, XPointer arg)
{
    return e->type == FocusIn && wanted_focusevent(e, FALSE);
}

static Bool event_look_for_focusin_client(Display *d, XEvent *e, XPointer arg)
{
    return e->type == FocusIn && wanted_focusevent(e, TRUE);
}

static void print_focusevent(XEvent *e)
{
    gint mode = e->xfocus.mode;
    gint detail = e->xfocus.detail;
    Window win = e->xany.window;
    const gchar *modestr, *detailstr;

    switch (mode) {
    case NotifyNormal:       modestr="NotifyNormal";       break;
    case NotifyGrab:         modestr="NotifyGrab";         break;
    case NotifyUngrab:       modestr="NotifyUngrab";       break;
    case NotifyWhileGrabbed: modestr="NotifyWhileGrabbed"; break;
    }
    switch (detail) {
    case NotifyAncestor:    detailstr="NotifyAncestor";    break;
    case NotifyVirtual:     detailstr="NotifyVirtual";     break;
    case NotifyInferior:    detailstr="NotifyInferior";    break;
    case NotifyNonlinear:   detailstr="NotifyNonlinear";   break;
    case NotifyNonlinearVirtual: detailstr="NotifyNonlinearVirtual"; break;
    case NotifyPointer:     detailstr="NotifyPointer";     break;
    case NotifyPointerRoot: detailstr="NotifyPointerRoot"; break;
    case NotifyDetailNone:  detailstr="NotifyDetailNone";  break;
    }

    if (mode == NotifyGrab || mode == NotifyUngrab)
        return;

    g_assert(modestr);
    g_assert(detailstr);
    ob_debug_type(OB_DEBUG_FOCUS, "Focus%s 0x%x mode=%s detail=%s\n",
                  (e->xfocus.type == FocusIn ? "In" : "Out"),
                  win,
                  modestr, detailstr);

}

static gboolean event_ignore(XEvent *e, ObClient *client)
{
    switch(e->type) {
    case FocusIn:
        print_focusevent(e);
        if (!wanted_focusevent(e, FALSE))
            return TRUE;
        break;
    case FocusOut:
        print_focusevent(e);
        if (!wanted_focusevent(e, FALSE))
            return TRUE;
        break;
    }
    return FALSE;
}

static void event_process(const XEvent *ec, gpointer data)
{
    Window window;
    ObClient *client = NULL;
    ObDock *dock = NULL;
    ObDockApp *dockapp = NULL;
    ObWindow *obwin = NULL;
    XEvent ee, *e;
    ObEventData *ed = data;
    ObPrompt *prompt = NULL;

    /* make a copy we can mangle */
    ee = *ec;
    e = &ee;

    window = event_get_window(e);
    if ((obwin = g_hash_table_lookup(window_map, &window))) {
        switch (obwin->type) {
        case Window_Dock:
            dock = WINDOW_AS_DOCK(obwin);
            break;
        case Window_DockApp:
            dockapp = WINDOW_AS_DOCKAPP(obwin);
            break;
        case Window_Client:
            client = WINDOW_AS_CLIENT(obwin);
            /* events on clients can be events on prompt windows too */
            prompt = client->prompt;
            break;
        case Window_Menu:
            /* not to be used for events */
            g_assert_not_reached();
            break;
        case Window_Internal:
            /* we don't do anything with events directly on these windows */
            break;
        case Window_Prompt:
            prompt = WINDOW_AS_PROMPT(obwin);
            break;
        }
    }

    event_set_curtime(e);
    event_curserial = e->xany.serial;
    event_hack_mods(e);
    if (event_ignore(e, client)) {
        if (ed)
            ed->ignored = TRUE;
        return;
    } else if (ed)
            ed->ignored = FALSE;

    /* deal with it in the kernel */

    if (menu_frame_visible &&
        (e->type == EnterNotify || e->type == LeaveNotify))
    {
        /* crossing events for menu */
        event_handle_menu(e);
    } else if (e->type == FocusIn) {
        if (client &&
            e->xfocus.detail == NotifyInferior)
        {
            ob_debug_type(OB_DEBUG_FOCUS,
                          "Focus went to the frame window");

            focus_left_screen = FALSE;

            focus_fallback(FALSE, config_focus_under_mouse, TRUE, TRUE);

            /* We don't get a FocusOut for this case, because it's just moving
               from our Inferior up to us. This happens when iconifying a
               window with RevertToParent focus */
            frame_adjust_focus(client->frame, FALSE);
            /* focus_set_client(NULL) has already been called */
        }
        else if (e->xfocus.detail == NotifyPointerRoot ||
                 e->xfocus.detail == NotifyDetailNone ||
                 e->xfocus.detail == NotifyInferior ||
                 e->xfocus.detail == NotifyNonlinear)
        {
            XEvent ce;

            ob_debug_type(OB_DEBUG_FOCUS,
                          "Focus went to root or pointer root/none\n");

            if (e->xfocus.detail == NotifyInferior ||
                e->xfocus.detail == NotifyNonlinear)
            {
                focus_left_screen = FALSE;
            }

            /* If another FocusIn is in the queue then don't fallback yet. This
               fixes the fun case of:
               window map -> send focusin
               window unmap -> get focusout
               window map -> send focusin
               get first focus out -> fall back to something (new window
                 hasn't received focus yet, so something else) -> send focusin
               which means the "something else" is the last thing to get a
               focusin sent to it, so the new window doesn't end up with focus.

               But if the other focus in is something like PointerRoot then we
               still want to fall back.
            */
            if (XCheckIfEvent(ob_display, &ce, event_look_for_focusin_client,
                              NULL))
            {
                XPutBackEvent(ob_display, &ce);
                ob_debug_type(OB_DEBUG_FOCUS,
                              "  but another FocusIn is coming\n");
            } else {
                /* Focus has been reverted.

                   FocusOut events come after UnmapNotify, so we don't need to
                   worry about focusing an invalid window
                */

                if (!focus_left_screen)
                    focus_fallback(FALSE, config_focus_under_mouse,
                                   TRUE, TRUE);
            }
        }
        else if (!client)
        {
            ob_debug_type(OB_DEBUG_FOCUS,
                          "Focus went to a window that is already gone\n");

            /* If you send focus to a window and then it disappears, you can
               get the FocusIn for it, after it is unmanaged.
               Just wait for the next FocusOut/FocusIn pair, but make note that
               the window that was focused no longer is. */
            focus_set_client(NULL);
        }
        else if (client != focus_client) {
            focus_left_screen = FALSE;
            frame_adjust_focus(client->frame, TRUE);
            focus_set_client(client);
            client_calc_layer(client);
            client_bring_helper_windows(client);
        }
    } else if (e->type == FocusOut) {
        XEvent ce;

        /* Look for the followup FocusIn */
        if (!XCheckIfEvent(ob_display, &ce, event_look_for_focusin, NULL)) {
            /* There is no FocusIn, this means focus went to a window that
               is not being managed, or a window on another screen. */
            Window win, root;
            gint i;
            guint u;
            xerror_set_ignore(TRUE);
            if (XGetInputFocus(ob_display, &win, &i) != 0 &&
                XGetGeometry(ob_display, win, &root, &i,&i,&u,&u,&u,&u) != 0 &&
                root != RootWindow(ob_display, ob_screen))
            {
                ob_debug_type(OB_DEBUG_FOCUS,
                              "Focus went to another screen !\n");
                focus_left_screen = TRUE;
            }
            else
                ob_debug_type(OB_DEBUG_FOCUS,
                              "Focus went to a black hole !\n");
            xerror_set_ignore(FALSE);
            /* nothing is focused */
            focus_set_client(NULL);
        } else {
            /* Focus moved, so process the FocusIn event */
            ObEventData ed = { .ignored = FALSE };
            event_process(&ce, &ed);
            if (ed.ignored) {
                /* The FocusIn was ignored, this means it was on a window
                   that isn't a client. */
                ob_debug_type(OB_DEBUG_FOCUS,
                              "Focus went to an unmanaged window 0x%x !\n",
                              ce.xfocus.window);
                focus_fallback(TRUE, config_focus_under_mouse, TRUE, TRUE);
            }
        }

        if (client && client != focus_client) {
            frame_adjust_focus(client->frame, FALSE);
            /* focus_set_client(NULL) has already been called in this
               section or by focus_fallback */
        }
    }
    else if (client)
        event_handle_client(client, e);
    else if (dockapp)
        event_handle_dockapp(dockapp, e);
    else if (dock)
        event_handle_dock(dock, e);
    else if (window == RootWindow(ob_display, ob_screen))
        event_handle_root(e);
    else if (e->type == MapRequest)
        client_manage(window, NULL);
    else if (e->type == MappingNotify) {
        /* keyboard layout changes for modifier mapping changes. reload the
           modifier map, and rebind all the key bindings as appropriate */
        ob_debug("Keyboard map changed. Reloading keyboard bindings.\n");
        XRefreshKeyboardMapping(&e->xmapping);
        ob_set_state(OB_STATE_RECONFIGURING);
        modkeys_shutdown(TRUE);
        modkeys_startup(TRUE);
        keyboard_rebind();
        ob_set_state(OB_STATE_RUNNING);
    }
    else if (e->type == ClientMessage) {
        /* This is for _NET_WM_REQUEST_FRAME_EXTENTS messages. They come for
           windows that are not managed yet. */
        if (e->xclient.message_type == prop_atoms.net_request_frame_extents) {
            /* Pretend to manage the client, getting information used to
               determine its decorations */
            ObClient *c = client_fake_manage(e->xclient.window);
            gulong vals[4];

            /* set the frame extents on the window */
            vals[0] = c->frame->size.left;
            vals[1] = c->frame->size.right;
            vals[2] = c->frame->size.top;
            vals[3] = c->frame->size.bottom;
            PROP_SETA32(e->xclient.window, net_frame_extents,
                        cardinal, vals, 4);

            /* Free the pretend client */
            client_fake_unmanage(c);
        }
    }
    else if (e->type == ConfigureRequest) {
        /* unhandled configure requests must be used to configure the
           window directly */
        XWindowChanges xwc;

        xwc.x = e->xconfigurerequest.x;
        xwc.y = e->xconfigurerequest.y;
        xwc.width = e->xconfigurerequest.width;
        xwc.height = e->xconfigurerequest.height;
        xwc.border_width = e->xconfigurerequest.border_width;
        xwc.sibling = e->xconfigurerequest.above;
        xwc.stack_mode = e->xconfigurerequest.detail;

        /* we are not to be held responsible if someone sends us an
           invalid request! */
        xerror_set_ignore(TRUE);
        XConfigureWindow(ob_display, window,
                         e->xconfigurerequest.value_mask, &xwc);
        xerror_set_ignore(FALSE);
    }
#ifdef SYNC
    else if (extensions_sync &&
        e->type == extensions_sync_event_basep + XSyncAlarmNotify)
    {
        XSyncAlarmNotifyEvent *se = (XSyncAlarmNotifyEvent*)e;
        if (se->alarm == moveresize_alarm && moveresize_in_progress)
            moveresize_event(e);
    }
#endif

    if (prompt && event_handle_prompt(prompt, e))
        ;
    else if (e->type == ButtonPress || e->type == ButtonRelease) {
        /* If the button press was on some non-root window, or was physically
           on the root window, then process it */
        if (window != RootWindow(ob_display, ob_screen) ||
            e->xbutton.subwindow == None)
        {
            event_handle_user_input(client, e);
        }
        /* Otherwise only process it if it was physically on an openbox
           internal window */
        else {
            ObWindow *w;

            if ((w = g_hash_table_lookup(window_map, &e->xbutton.subwindow)) &&
                WINDOW_IS_INTERNAL(w))
            {
                event_handle_user_input(client, e);
            }
        }
    }
    else if (e->type == KeyPress || e->type == KeyRelease ||
             e->type == MotionNotify)
        event_handle_user_input(client, e);

    /* if something happens and it's not from an XEvent, then we don't know
       the time */
    event_curtime = CurrentTime;
    event_curserial = 0;
}

static void event_handle_root(XEvent *e)
{
    Atom msgtype;

    switch(e->type) {
    case SelectionClear:
        ob_debug("Another WM has requested to replace us. Exiting.\n");
        ob_exit_replace();
        break;

    case ClientMessage:
        if (e->xclient.format != 32) break;

        msgtype = e->xclient.message_type;
        if (msgtype == prop_atoms.net_current_desktop) {
            guint d = e->xclient.data.l[0];
            if (d < screen_num_desktops) {
                event_curtime = e->xclient.data.l[1];
                if (event_curtime == 0)
                    ob_debug_type(OB_DEBUG_APP_BUGS,
                                  "_NET_CURRENT_DESKTOP message is missing "
                                  "a timestamp\n");
                screen_set_desktop(d, TRUE);
            }
        } else if (msgtype == prop_atoms.net_number_of_desktops) {
            guint d = e->xclient.data.l[0];
            if (d > 0 && d <= 1000)
                screen_set_num_desktops(d);
        } else if (msgtype == prop_atoms.net_showing_desktop) {
            screen_show_desktop(e->xclient.data.l[0] != 0, NULL);
        } else if (msgtype == prop_atoms.ob_control) {
            ob_debug("OB_CONTROL: %d\n", e->xclient.data.l[0]);
            if (e->xclient.data.l[0] == 1)
                ob_reconfigure();
            else if (e->xclient.data.l[0] == 2)
                ob_restart();
            else if (e->xclient.data.l[0] == 3)
                ob_exit(0);
        } else if (msgtype == prop_atoms.wm_protocols) {
            if ((Atom)e->xclient.data.l[0] == prop_atoms.net_wm_ping)
                ping_got_pong(e->xclient.data.l[1]);
        }
        break;
    case PropertyNotify:
        if (e->xproperty.atom == prop_atoms.net_desktop_names) {
            ob_debug("UPDATE DESKTOP NAMES\n");
            screen_update_desktop_names();
        }
        else if (e->xproperty.atom == prop_atoms.net_desktop_layout)
            screen_update_layout();
        break;
    case ConfigureNotify:
#ifdef XRANDR
        XRRUpdateConfiguration(e);
#endif
        screen_resize();
        break;
    default:
        ;
    }
}

void event_enter_client(ObClient *client)
{
    g_assert(config_focus_follow);

    if (is_enter_focus_event_ignored(event_curserial)) {
        ob_debug_type(OB_DEBUG_FOCUS, "Ignoring enter event with serial %lu\n"
                      "on client 0x%x", event_curserial, client->window);
        return;
    }

    if (client_enter_focusable(client) && client_can_focus(client)) {
        if (config_focus_delay) {
            ObFocusDelayData *data;

            ob_main_loop_timeout_remove(ob_main_loop, focus_delay_func);

            data = g_new(ObFocusDelayData, 1);
            data->client = client;
            data->time = event_curtime;
            data->serial = event_curserial;

            ob_main_loop_timeout_add(ob_main_loop,
                                     config_focus_delay * 1000,
                                     focus_delay_func,
                                     data, focus_delay_cmp, focus_delay_dest);
        } else {
            ObFocusDelayData data;
            data.client = client;
            data.time = event_curtime;
            data.serial = event_curserial;
            focus_delay_func(&data);
        }
    }
}

static gboolean *context_to_button(ObFrame *f, ObFrameContext con, gboolean press)
{
    if (press) {
        switch (con) {
        case OB_FRAME_CONTEXT_MAXIMIZE:
            return &f->max_press;
        case OB_FRAME_CONTEXT_CLOSE:
            return &f->close_press;
        case OB_FRAME_CONTEXT_ICONIFY:
            return &f->iconify_press;
        case OB_FRAME_CONTEXT_ALLDESKTOPS:
            return &f->desk_press;
        case OB_FRAME_CONTEXT_SHADE:
            return &f->shade_press;
        default:
            return NULL;
        }
    } else {
        switch (con) {
        case OB_FRAME_CONTEXT_MAXIMIZE:
            return &f->max_hover;
        case OB_FRAME_CONTEXT_CLOSE:
            return &f->close_hover;
        case OB_FRAME_CONTEXT_ICONIFY:
            return &f->iconify_hover;
        case OB_FRAME_CONTEXT_ALLDESKTOPS:
            return &f->desk_hover;
        case OB_FRAME_CONTEXT_SHADE:
            return &f->shade_hover;
        default:
            return NULL;
        }
    }
}

static void compress_client_message_event(XEvent *e, XEvent *ce, Window window,
                                          Atom msgtype)
{
    /* compress changes into a single change */
    while (XCheckTypedWindowEvent(ob_display, window, e->type, ce)) {
        /* XXX: it would be nice to compress ALL messages of a
           type, not just messages in a row without other
           message types between. */
        if (ce->xclient.message_type != msgtype) {
            XPutBackEvent(ob_display, ce);
            break;
        }
        e->xclient = ce->xclient;
    }
}

static void event_handle_client(ObClient *client, XEvent *e)
{
    XEvent ce;
    Atom msgtype;
    ObFrameContext con;
    gboolean *but;
    static gint px = -1, py = -1;
    static guint pb = 0;
    static ObFrameContext pcon = OB_FRAME_CONTEXT_NONE;

    switch (e->type) {
    case ButtonPress:
        /* save where the press occured for the first button pressed */
        if (!pb) {
            pb = e->xbutton.button;
            px = e->xbutton.x;
            py = e->xbutton.y;

            pcon = frame_context(client, e->xbutton.window, px, py);
            pcon = mouse_button_frame_context(pcon, e->xbutton.button,
                                              e->xbutton.state);
        }
    case ButtonRelease:
        /* Wheel buttons don't draw because they are an instant click, so it
           is a waste of resources to go drawing it.
           if the user is doing an interactive thing, or has a menu open then
           the mouse is grabbed (possibly) and if we get these events we don't
           want to deal with them
        */
        if (!(e->xbutton.button == 4 || e->xbutton.button == 5) &&
            !grab_on_keyboard())
        {
            /* use where the press occured */
            con = frame_context(client, e->xbutton.window, px, py);
            con = mouse_button_frame_context(con, e->xbutton.button,
                                             e->xbutton.state);

            /* button presses on CLIENT_CONTEXTs are not accompanied by a
               release because they are Replayed to the client */
            if ((e->type == ButtonRelease || CLIENT_CONTEXT(con, client)) &&
                e->xbutton.button == pb)
                pb = 0, px = py = -1, pcon = OB_FRAME_CONTEXT_NONE;

            but = context_to_button(client->frame, con, TRUE);
            if (but) {
                *but = (e->type == ButtonPress);
                frame_adjust_state(client->frame);
            }
        }
        break;
    case MotionNotify:
        /* when there is a grab on the pointer, we won't get enter/leave
           notifies, but we still get motion events */
        if (grab_on_pointer()) break;

        con = frame_context(client, e->xmotion.window,
                            e->xmotion.x, e->xmotion.y);
        switch (con) {
        case OB_FRAME_CONTEXT_TITLEBAR:
        case OB_FRAME_CONTEXT_TLCORNER:
        case OB_FRAME_CONTEXT_TRCORNER:
            /* we've left the button area inside the titlebar */
            if (client->frame->max_hover || client->frame->desk_hover ||
                client->frame->shade_hover || client->frame->iconify_hover ||
                client->frame->close_hover)
            {
                client->frame->max_hover =
                    client->frame->desk_hover =
                    client->frame->shade_hover =
                    client->frame->iconify_hover =
                    client->frame->close_hover = FALSE;
                frame_adjust_state(client->frame);
            }
            break;
        default:
            but = context_to_button(client->frame, con, FALSE);
            if (but && !*but && !pb) {
                *but = TRUE;
                frame_adjust_state(client->frame);
            }
            break;
        }
        break;
    case LeaveNotify:
        con = frame_context(client, e->xcrossing.window,
                            e->xcrossing.x, e->xcrossing.y);
        switch (con) {
        case OB_FRAME_CONTEXT_TITLEBAR:
        case OB_FRAME_CONTEXT_TLCORNER:
        case OB_FRAME_CONTEXT_TRCORNER:
            /* we've left the button area inside the titlebar */
            client->frame->max_hover =
                client->frame->desk_hover =
                client->frame->shade_hover =
                client->frame->iconify_hover =
                client->frame->close_hover = FALSE;
            if (e->xcrossing.mode == NotifyGrab) {
                client->frame->max_press =
                    client->frame->desk_press =
                    client->frame->shade_press =
                    client->frame->iconify_press =
                    client->frame->close_press = FALSE;
            }
            break;
        case OB_FRAME_CONTEXT_FRAME:
            /* When the mouse leaves an animating window, don't use the
               corresponding enter events. Pretend like the animating window
               doesn't even exist..! */
            if (frame_iconify_animating(client->frame))
                event_end_ignore_all_enters(event_start_ignore_all_enters());

            ob_debug_type(OB_DEBUG_FOCUS,
                          "%sNotify mode %d detail %d on %lx\n",
                          (e->type == EnterNotify ? "Enter" : "Leave"),
                          e->xcrossing.mode,
                          e->xcrossing.detail, (client?client->window:0));
            if (grab_on_keyboard())
                break;
            if (config_focus_follow && config_focus_delay &&
                /* leave inferior events can happen when the mouse goes onto
                   the window's border and then into the window before the
                   delay is up */
                e->xcrossing.detail != NotifyInferior)
            {
                ob_main_loop_timeout_remove_data(ob_main_loop,
                                                 focus_delay_func,
                                                 client, FALSE);
            }
            break;
        default:
            but = context_to_button(client->frame, con, FALSE);
            if (but) {
                *but = FALSE;
                if (e->xcrossing.mode == NotifyGrab) {
                    but = context_to_button(client->frame, con, TRUE);
                    *but = FALSE;
                }
                frame_adjust_state(client->frame);
            }
            break;
        }
        break;
    case EnterNotify:
    {
        con = frame_context(client, e->xcrossing.window,
                            e->xcrossing.x, e->xcrossing.y);
        switch (con) {
        case OB_FRAME_CONTEXT_FRAME:
            if (grab_on_keyboard())
                break;
            if (e->xcrossing.mode == NotifyGrab ||
                e->xcrossing.mode == NotifyUngrab ||
                /*ignore enters when we're already in the window */
                e->xcrossing.detail == NotifyInferior)
            {
                ob_debug_type(OB_DEBUG_FOCUS,
                              "%sNotify mode %d detail %d serial %lu on %lx "
                              "IGNORED\n",
                              (e->type == EnterNotify ? "Enter" : "Leave"),
                              e->xcrossing.mode,
                              e->xcrossing.detail,
                              e->xcrossing.serial,
                              client?client->window:0);
            }
            else {
                ob_debug_type(OB_DEBUG_FOCUS,
                              "%sNotify mode %d detail %d serial %lu on %lx, "
                              "focusing window\n",
                              (e->type == EnterNotify ? "Enter" : "Leave"),
                              e->xcrossing.mode,
                              e->xcrossing.detail,
                              e->xcrossing.serial,
                              (client?client->window:0));
                if (config_focus_follow)
                    event_enter_client(client);
            }
            break;
        default:
            but = context_to_button(client->frame, con, FALSE);
            if (but) {
                *but = TRUE;
                if (e->xcrossing.mode == NotifyUngrab) {
                    but = context_to_button(client->frame, con, TRUE);
                    *but = (con == pcon);
                }
                frame_adjust_state(client->frame);
            }
            break;
        }
        break;
    }
    case ConfigureRequest:
    {
        /* dont compress these unless you're going to watch for property
           notifies in between (these can change what the configure would
           do to the window).
           also you can't compress stacking events
        */

        gint x, y, w, h;
        gboolean move = FALSE;
        gboolean resize = FALSE;

        /* get the current area */
        RECT_TO_DIMS(client->area, x, y, w, h);

        ob_debug("ConfigureRequest for \"%s\" desktop %d wmstate %d "
                 "visible %d\n"
                 "                     x %d y %d w %d h %d b %d\n",
                 client->title,
                 screen_desktop, client->wmstate, client->frame->visible,
                 x, y, w, h, client->border_width);

        if (e->xconfigurerequest.value_mask & CWBorderWidth)
            if (client->border_width != e->xconfigurerequest.border_width) {
                client->border_width = e->xconfigurerequest.border_width;

                /* if the border width is changing then that is the same
                   as requesting a resize, but we don't actually change
                   the client's border, so it will change their root
                   coordinates (since they include the border width) and
                   we need to a notify then */
                move = TRUE;
            }

        if (e->xconfigurerequest.value_mask & CWStackMode) {
            ObClient *sibling = NULL;
            gulong ignore_start;
            gboolean ok = TRUE;

            /* get the sibling */
            if (e->xconfigurerequest.value_mask & CWSibling) {
                ObWindow *win;
                win = g_hash_table_lookup(window_map,
                                          &e->xconfigurerequest.above);
                if (win && WINDOW_IS_CLIENT(win) &&
                    WINDOW_AS_CLIENT(win) != client)
                {
                    sibling = WINDOW_AS_CLIENT(win);
                }
                else
                    /* an invalid sibling was specified so don't restack at
                       all, it won't make sense no matter what we do */
                    ok = FALSE;
            }

            if (ok) {
                if (!config_focus_under_mouse)
                    ignore_start = event_start_ignore_all_enters();
                stacking_restack_request(client, sibling,
                                         e->xconfigurerequest.detail);
                if (!config_focus_under_mouse)
                    event_end_ignore_all_enters(ignore_start);
            }

            /* a stacking change moves the window without resizing */
            move = TRUE;
        }

        if ((e->xconfigurerequest.value_mask & CWX) ||
            (e->xconfigurerequest.value_mask & CWY) ||
            (e->xconfigurerequest.value_mask & CWWidth) ||
            (e->xconfigurerequest.value_mask & CWHeight))
        {
            /* don't allow clients to move shaded windows (fvwm does this)
            */
            if (e->xconfigurerequest.value_mask & CWX) {
                if (!client->shaded)
                    x = e->xconfigurerequest.x;
                move = TRUE;
            }
            if (e->xconfigurerequest.value_mask & CWY) {
                if (!client->shaded)
                    y = e->xconfigurerequest.y;
                move = TRUE;
            }

            if (e->xconfigurerequest.value_mask & CWWidth) {
                w = e->xconfigurerequest.width;
                resize = TRUE;
            }
            if (e->xconfigurerequest.value_mask & CWHeight) {
                h = e->xconfigurerequest.height;
                resize = TRUE;
            }
        }

        ob_debug("ConfigureRequest x(%d) %d y(%d) %d w(%d) %d h(%d) %d "
                 "move %d resize %d\n",
                 e->xconfigurerequest.value_mask & CWX, x,
                 e->xconfigurerequest.value_mask & CWY, y,
                 e->xconfigurerequest.value_mask & CWWidth, w,
                 e->xconfigurerequest.value_mask & CWHeight, h,
                 move, resize);

        /* check for broken apps moving to their root position

           XXX remove this some day...that would be nice. right now all
           kde apps do this when they try activate themselves on another
           desktop. eg. open amarok window on desktop 1, switch to desktop
           2, click amarok tray icon. it will move by its decoration size.
        */
        if (x != client->area.x &&
            x == (client->frame->area.x + client->frame->size.left -
                  (gint)client->border_width) &&
            y != client->area.y &&
            y == (client->frame->area.y + client->frame->size.top -
                  (gint)client->border_width) &&
            w == client->area.width &&
            h == client->area.height)
        {
            ob_debug_type(OB_DEBUG_APP_BUGS,
                          "Application %s is trying to move via "
                          "ConfigureRequest to it's root window position "
                          "but it is not using StaticGravity\n",
                          client->title);
            /* don't move it */
            x = client->area.x;
            y = client->area.y;

            /* they still requested a move, so don't change whether a
               notify is sent or not */
        }

        {
            gint lw, lh;

            client_try_configure(client, &x, &y, &w, &h, &lw, &lh, FALSE);

            /* if x was not given, then use gravity to figure out the new
               x.  the reference point should not be moved */
            if ((e->xconfigurerequest.value_mask & CWWidth &&
                 !(e->xconfigurerequest.value_mask & CWX)))
                client_gravity_resize_w(client, &x, client->area.width, w);
            /* same for y */
            if ((e->xconfigurerequest.value_mask & CWHeight &&
                 !(e->xconfigurerequest.value_mask & CWY)))
                client_gravity_resize_h(client, &y, client->area.height,h);

            client_find_onscreen(client, &x, &y, w, h, FALSE);

            ob_debug("Granting ConfigureRequest x %d y %d w %d h %d\n",
                     x, y, w, h);
            client_configure(client, x, y, w, h, FALSE, TRUE, TRUE);
        }
        break;
    }
    case UnmapNotify:
        if (client->ignore_unmaps) {
            client->ignore_unmaps--;
            break;
        }
        ob_debug("UnmapNotify for window 0x%x eventwin 0x%x sendevent %d "
                 "ignores left %d\n",
                 client->window, e->xunmap.event, e->xunmap.from_configure,
                 client->ignore_unmaps);
        client_unmanage(client);
        break;
    case DestroyNotify:
        ob_debug("DestroyNotify for window 0x%x\n", client->window);
        client_unmanage(client);
        break;
    case ReparentNotify:
        /* this is when the client is first taken captive in the frame */
        if (e->xreparent.parent == client->frame->window) break;

        /*
          This event is quite rare and is usually handled in unmapHandler.
          However, if the window is unmapped when the reparent event occurs,
          the window manager never sees it because an unmap event is not sent
          to an already unmapped window.
        */

        /* we don't want the reparent event, put it back on the stack for the
           X server to deal with after we unmanage the window */
        XPutBackEvent(ob_display, e);

        ob_debug("ReparentNotify for window 0x%x\n", client->window);
        client_unmanage(client);
        break;
    case MapRequest:
        ob_debug("MapRequest for 0x%lx\n", client->window);
        if (!client->iconic) break; /* this normally doesn't happen, but if it
                                       does, we don't want it!
                                       it can happen now when the window is on
                                       another desktop, but we still don't
                                       want it! */
        client_activate(client, FALSE, FALSE, TRUE, TRUE, TRUE);
        break;
    case ClientMessage:
        /* validate cuz we query stuff off the client here */
        if (!client_validate(client)) break;

        if (e->xclient.format != 32) return;

        msgtype = e->xclient.message_type;
        if (msgtype == prop_atoms.wm_change_state) {
            compress_client_message_event(e, &ce, client->window, msgtype);
            client_set_wm_state(client, e->xclient.data.l[0]);
        } else if (msgtype == prop_atoms.net_wm_desktop) {
            compress_client_message_event(e, &ce, client->window, msgtype);
            if ((unsigned)e->xclient.data.l[0] < screen_num_desktops ||
                (unsigned)e->xclient.data.l[0] == DESKTOP_ALL)
                client_set_desktop(client, (unsigned)e->xclient.data.l[0],
                                   FALSE, FALSE);
        } else if (msgtype == prop_atoms.net_wm_state) {
            gulong ignore_start;

            /* can't compress these */
            ob_debug("net_wm_state %s %ld %ld for 0x%lx\n",
                     (e->xclient.data.l[0] == 0 ? "Remove" :
                      e->xclient.data.l[0] == 1 ? "Add" :
                      e->xclient.data.l[0] == 2 ? "Toggle" : "INVALID"),
                     e->xclient.data.l[1], e->xclient.data.l[2],
                     client->window);

            /* ignore enter events caused by these like ob actions do */
            if (!config_focus_under_mouse)
                ignore_start = event_start_ignore_all_enters();
            client_set_state(client, e->xclient.data.l[0],
                             e->xclient.data.l[1], e->xclient.data.l[2]);
            if (!config_focus_under_mouse)
                event_end_ignore_all_enters(ignore_start);
        } else if (msgtype == prop_atoms.net_close_window) {
            ob_debug("net_close_window for 0x%lx\n", client->window);
            client_close(client);
        } else if (msgtype == prop_atoms.net_active_window) {
            ob_debug("net_active_window for 0x%lx source=%s\n",
                     client->window,
                     (e->xclient.data.l[0] == 0 ? "unknown" :
                      (e->xclient.data.l[0] == 1 ? "application" :
                       (e->xclient.data.l[0] == 2 ? "user" : "INVALID"))));
            /* XXX make use of data.l[2] !? */
            if (e->xclient.data.l[0] == 1 || e->xclient.data.l[0] == 2) {
                /* we can not trust the timestamp from applications.
                   e.g. chromium passes a very old timestamp.  openbox thinks
                   the window will get focus and calls XSetInputFocus with the
                   (old) timestamp, which doesn't end up moving focus at all.
                   but the window is raised, not hilited, etc, as if it was
                   really going to get focus.

                   so do not use this timestamp in event_curtime, as this would
                   be used in XSetInputFocus.
                */
                /*event_curtime = e->xclient.data.l[1];*/
                if (e->xclient.data.l[1] == 0)
                    ob_debug_type(OB_DEBUG_APP_BUGS,
                                  "_NET_ACTIVE_WINDOW message for window %s is"
                                  " missing a timestamp\n", client->title);

                event_curtime = event_get_server_time();
            } else
                ob_debug_type(OB_DEBUG_APP_BUGS,
                              "_NET_ACTIVE_WINDOW message for window %s is "
                              "missing source indication\n", client->title);
            client_activate(client, FALSE, FALSE, TRUE, TRUE,
                            (e->xclient.data.l[0] == 0 ||
                             e->xclient.data.l[0] == 2));
        } else if (msgtype == prop_atoms.net_wm_moveresize) {
            ob_debug("net_wm_moveresize for 0x%lx direction %d\n",
                     client->window, e->xclient.data.l[2]);
            if ((Atom)e->xclient.data.l[2] ==
                prop_atoms.net_wm_moveresize_size_topleft ||
                (Atom)e->xclient.data.l[2] ==
                prop_atoms.net_wm_moveresize_size_top ||
                (Atom)e->xclient.data.l[2] ==
                prop_atoms.net_wm_moveresize_size_topright ||
                (Atom)e->xclient.data.l[2] ==
                prop_atoms.net_wm_moveresize_size_right ||
                (Atom)e->xclient.data.l[2] ==
                prop_atoms.net_wm_moveresize_size_right ||
                (Atom)e->xclient.data.l[2] ==
                prop_atoms.net_wm_moveresize_size_bottomright ||
                (Atom)e->xclient.data.l[2] ==
                prop_atoms.net_wm_moveresize_size_bottom ||
                (Atom)e->xclient.data.l[2] ==
                prop_atoms.net_wm_moveresize_size_bottomleft ||
                (Atom)e->xclient.data.l[2] ==
                prop_atoms.net_wm_moveresize_size_left ||
                (Atom)e->xclient.data.l[2] ==
                prop_atoms.net_wm_moveresize_move ||
                (Atom)e->xclient.data.l[2] ==
                prop_atoms.net_wm_moveresize_size_keyboard ||
                (Atom)e->xclient.data.l[2] ==
                prop_atoms.net_wm_moveresize_move_keyboard) {

                moveresize_start(client, e->xclient.data.l[0],
                                 e->xclient.data.l[1], e->xclient.data.l[3],
                                 e->xclient.data.l[2]);
            }
            else if ((Atom)e->xclient.data.l[2] ==
                     prop_atoms.net_wm_moveresize_cancel)
                moveresize_end(TRUE);
        } else if (msgtype == prop_atoms.net_moveresize_window) {
            gint ograv, x, y, w, h;

            ograv = client->gravity;

            if (e->xclient.data.l[0] & 0xff)
                client->gravity = e->xclient.data.l[0] & 0xff;

            if (e->xclient.data.l[0] & 1 << 8)
                x = e->xclient.data.l[1];
            else
                x = client->area.x;
            if (e->xclient.data.l[0] & 1 << 9)
                y = e->xclient.data.l[2];
            else
                y = client->area.y;

            if (e->xclient.data.l[0] & 1 << 10) {
                w = e->xclient.data.l[3];

                /* if x was not given, then use gravity to figure out the new
                   x.  the reference point should not be moved */
                if (!(e->xclient.data.l[0] & 1 << 8))
                    client_gravity_resize_w(client, &x, client->area.width, w);
            }
            else
                w = client->area.width;

            if (e->xclient.data.l[0] & 1 << 11) {
                h = e->xclient.data.l[4];

                /* same for y */
                if (!(e->xclient.data.l[0] & 1 << 9))
                    client_gravity_resize_h(client, &y, client->area.height,h);
            }
            else
                h = client->area.height;

            ob_debug("MOVERESIZE x %d %d y %d %d (gravity %d)\n",
                     e->xclient.data.l[0] & 1 << 8, x,
                     e->xclient.data.l[0] & 1 << 9, y,
                     client->gravity);

            client_find_onscreen(client, &x, &y, w, h, FALSE);

            client_configure(client, x, y, w, h, FALSE, TRUE, FALSE);

            client->gravity = ograv;
        } else if (msgtype == prop_atoms.net_restack_window) {
            if (e->xclient.data.l[0] != 2) {
                ob_debug_type(OB_DEBUG_APP_BUGS,
                              "_NET_RESTACK_WINDOW sent for window %s with "
                              "invalid source indication %ld\n",
                              client->title, e->xclient.data.l[0]);
            } else {
                ObClient *sibling = NULL;
                if (e->xclient.data.l[1]) {
                    ObWindow *win = g_hash_table_lookup
                        (window_map, &e->xclient.data.l[1]);
                    if (WINDOW_IS_CLIENT(win) &&
                        WINDOW_AS_CLIENT(win) != client)
                    {
                        sibling = WINDOW_AS_CLIENT(win);
                    }
                    if (sibling == NULL)
                        ob_debug_type(OB_DEBUG_APP_BUGS,
                                      "_NET_RESTACK_WINDOW sent for window %s "
                                      "with invalid sibling 0x%x\n",
                                 client->title, e->xclient.data.l[1]);
                }
                if (e->xclient.data.l[2] == Below ||
                    e->xclient.data.l[2] == BottomIf ||
                    e->xclient.data.l[2] == Above ||
                    e->xclient.data.l[2] == TopIf ||
                    e->xclient.data.l[2] == Opposite)
                {
                    gulong ignore_start;

                    if (!config_focus_under_mouse)
                        ignore_start = event_start_ignore_all_enters();
                    /* just raise, don't activate */
                    stacking_restack_request(client, sibling,
                                             e->xclient.data.l[2]);
                    if (!config_focus_under_mouse)
                        event_end_ignore_all_enters(ignore_start);

                    /* send a synthetic ConfigureNotify, cuz this is supposed
                       to be like a ConfigureRequest. */
                    client_reconfigure(client, TRUE);
                } else
                    ob_debug_type(OB_DEBUG_APP_BUGS,
                                  "_NET_RESTACK_WINDOW sent for window %s "
                                  "with invalid detail %d\n",
                                  client->title, e->xclient.data.l[2]);
            }
        }
        break;
    case PropertyNotify:
        /* validate cuz we query stuff off the client here */
        if (!client_validate(client)) break;

        /* compress changes to a single property into a single change */
        while (XCheckTypedWindowEvent(ob_display, client->window,
                                      e->type, &ce)) {
            Atom a, b;

            /* XXX: it would be nice to compress ALL changes to a property,
               not just changes in a row without other props between. */

            a = ce.xproperty.atom;
            b = e->xproperty.atom;

            if (a == b)
                continue;
            if ((a == prop_atoms.net_wm_name ||
                 a == prop_atoms.wm_name ||
                 a == prop_atoms.net_wm_icon_name ||
                 a == prop_atoms.wm_icon_name)
                &&
                (b == prop_atoms.net_wm_name ||
                 b == prop_atoms.wm_name ||
                 b == prop_atoms.net_wm_icon_name ||
                 b == prop_atoms.wm_icon_name)) {
                continue;
            }
            if (a == prop_atoms.net_wm_icon &&
                b == prop_atoms.net_wm_icon)
                continue;

            XPutBackEvent(ob_display, &ce);
            break;
        }

        msgtype = e->xproperty.atom;
        if (msgtype == XA_WM_NORMAL_HINTS) {
            ob_debug("Update NORMAL hints\n");
            client_update_normal_hints(client);
            /* normal hints can make a window non-resizable */
            client_setup_decor_and_functions(client, FALSE);

            /* make sure the client's sizes are within its bounds, but only
               reconfigure the window if it needs to. emacs will update its
               normal hints every time it receives a conigurenotify */
            client_reconfigure(client, FALSE);
        } else if (msgtype == prop_atoms.motif_wm_hints) {
            client_get_mwm_hints(client);
            /* This can override some mwm hints */
            client_get_type_and_transientness(client);

            /* Apply the changes to the window */
            client_setup_decor_and_functions(client, TRUE);
        } else if (msgtype == XA_WM_HINTS) {
            client_update_wmhints(client);
        } else if (msgtype == XA_WM_TRANSIENT_FOR) {
            client_update_transient_for(client);
            client_get_type_and_transientness(client);
            /* type may have changed, so update the layer */
            client_calc_layer(client);
            client_setup_decor_and_functions(client, TRUE);
        } else if (msgtype == prop_atoms.net_wm_name ||
                   msgtype == prop_atoms.wm_name ||
                   msgtype == prop_atoms.net_wm_icon_name ||
                   msgtype == prop_atoms.wm_icon_name) {
            client_update_title(client);
        } else if (msgtype == prop_atoms.wm_protocols) {
            client_update_protocols(client);
            client_setup_decor_and_functions(client, TRUE);
        }
        else if (msgtype == prop_atoms.net_wm_strut ||
                 msgtype == prop_atoms.net_wm_strut_partial) {
            client_update_strut(client);
        }
        else if (msgtype == prop_atoms.net_wm_icon) {
            client_update_icons(client);
        }
        else if (msgtype == prop_atoms.net_wm_icon_geometry) {
            client_update_icon_geometry(client);
        }
        else if (msgtype == prop_atoms.net_wm_user_time) {
            guint32 t;
            if (client == focus_client &&
                PROP_GET32(client->window, net_wm_user_time, cardinal, &t) &&
                t && !event_time_after(t, e->xproperty.time) &&
                (!event_last_user_time ||
                 event_time_after(t, event_last_user_time)))
            {
                event_last_user_time = t;
            }
        }
#ifdef SYNC
        else if (msgtype == prop_atoms.net_wm_sync_request_counter) {
            client_update_sync_request_counter(client);
        }
#endif
        break;
    case ColormapNotify:
        client_update_colormap(client, e->xcolormap.colormap);
        break;
    default:
        ;
#ifdef SHAPE
        {
            int kind;
            if (extensions_shape && e->type == extensions_shape_event_basep) {
                switch (((XShapeEvent*)e)->kind) {
                    case ShapeBounding:
                    case ShapeClip:
                        client->shaped = ((XShapeEvent*)e)->shaped;
                        kind = ShapeBounding;
                        break;
                    case ShapeInput:
                        client->shaped_input = ((XShapeEvent*)e)->shaped;
                        kind = ShapeInput;
                        break;
                }
                frame_adjust_shape_kind(client->frame, kind);
            }
        }
#endif
    }
}

static void event_handle_dock(ObDock *s, XEvent *e)
{
    switch (e->type) {
    case ButtonPress:
        if (e->xbutton.button == 1)
            stacking_raise(DOCK_AS_WINDOW(s));
        else if (e->xbutton.button == 2)
            stacking_lower(DOCK_AS_WINDOW(s));
        break;
    case EnterNotify:
        dock_hide(FALSE);
        break;
    case LeaveNotify:
        /* don't hide when moving into a dock app */
        if (e->xcrossing.detail != NotifyInferior)
            dock_hide(TRUE);
        break;
    }
}

static void event_handle_dockapp(ObDockApp *app, XEvent *e)
{
    switch (e->type) {
    case MotionNotify:
        dock_app_drag(app, &e->xmotion);
        break;
    case UnmapNotify:
        if (app->ignore_unmaps) {
            app->ignore_unmaps--;
            break;
        }
        dock_remove(app, TRUE);
        break;
    case DestroyNotify:
    case ReparentNotify:
        dock_remove(app, FALSE);
        break;
    case ConfigureNotify:
        dock_app_configure(app, e->xconfigure.width, e->xconfigure.height);
        break;
    }
}

static ObMenuFrame* find_active_menu(void)
{
    GList *it;
    ObMenuFrame *ret = NULL;

    for (it = menu_frame_visible; it; it = g_list_next(it)) {
        ret = it->data;
        if (ret->selected)
            break;
        ret = NULL;
    }
    return ret;
}

static ObMenuFrame* find_active_or_last_menu(void)
{
    ObMenuFrame *ret = NULL;

    ret = find_active_menu();
    if (!ret && menu_frame_visible)
        ret = menu_frame_visible->data;
    return ret;
}

static gboolean event_handle_prompt(ObPrompt *p, XEvent *e)
{
    switch (e->type) {
    case ButtonPress:
    case ButtonRelease:
    case MotionNotify:
        return prompt_mouse_event(p, e);
        break;
    case KeyPress:
        return prompt_key_event(p, e);
        break;
    }
    return FALSE;
}

static gboolean event_handle_menu_keyboard(XEvent *ev)
{
    guint keycode, state;
    gunichar unikey;
    ObMenuFrame *frame;
    gboolean ret = FALSE;

    keycode = ev->xkey.keycode;
    state = ev->xkey.state;
    unikey = translate_unichar(keycode);

    frame = find_active_or_last_menu();
    if (frame == NULL)
        g_assert_not_reached(); /* there is no active menu */

    /* Allow control while going thru the menu */
    else if (ev->type == KeyPress && (state & ~ControlMask) == 0) {
        frame->got_press = TRUE;

        if (ob_keycode_match(keycode, OB_KEY_ESCAPE)) {
            menu_frame_hide_all();
            ret = TRUE;
        }

        else if (ob_keycode_match(keycode, OB_KEY_LEFT)) {
            /* Left goes to the parent menu */
            if (frame->parent) {
                /* remove focus from the child */
                menu_frame_select(frame, NULL, TRUE);
                /* and put it in the parent */
                menu_frame_select(frame->parent, frame->parent->selected,
                                  TRUE);
            }
            ret = TRUE;
        }

        else if (ob_keycode_match(keycode, OB_KEY_RIGHT)) {
            /* Right goes to the selected submenu */
            if (frame->selected &&
                frame->selected->entry->type == OB_MENU_ENTRY_TYPE_SUBMENU)
            {
                /* make sure it is visible */
                menu_frame_select(frame, frame->selected, TRUE);
                menu_frame_select_next(frame->child);
            }
            ret = TRUE;
        }

        else if (ob_keycode_match(keycode, OB_KEY_UP)) {
            menu_frame_select_previous(frame);
            ret = TRUE;
        }

        else if (ob_keycode_match(keycode, OB_KEY_DOWN)) {
            menu_frame_select_next(frame);
            ret = TRUE;
        }

        else if (ob_keycode_match(keycode, OB_KEY_HOME)) {
            menu_frame_select_first(frame);
            ret = TRUE;
        }

        else if (ob_keycode_match(keycode, OB_KEY_END)) {
            menu_frame_select_last(frame);
            ret = TRUE;
        }
    }

    /* Use KeyRelease events for running things so that the key release doesn't
       get sent to the focused application.

       Allow ControlMask only, and don't bother if the menu is empty */
    else if (ev->type == KeyRelease && (state & ~ControlMask) == 0 &&
             frame->entries && frame->got_press)
    {
        if (ob_keycode_match(keycode, OB_KEY_RETURN)) {
            /* Enter runs the active item or goes into the submenu.
               Control-Enter runs it without closing the menu. */
            if (frame->child)
                menu_frame_select_next(frame->child);
            else if (frame->selected)
                menu_entry_frame_execute(frame->selected, state);

            ret = TRUE;
        }

        /* keyboard accelerator shortcuts. (if it was a valid key) */
        else if (unikey != 0) {
            GList *start;
            GList *it;
            ObMenuEntryFrame *found = NULL;
            guint num_found = 0;

            /* start after the selected one */
            start = frame->entries;
            if (frame->selected) {
                for (it = start; frame->selected != it->data;
                     it = g_list_next(it))
                    g_assert(it != NULL); /* nothing was selected? */
                /* next with wraparound */
                start = g_list_next(it);
                if (start == NULL) start = frame->entries;
            }

            it = start;
            do {
                ObMenuEntryFrame *e = it->data;
                gunichar entrykey = 0;

                if (e->entry->type == OB_MENU_ENTRY_TYPE_NORMAL)
                    entrykey = e->entry->data.normal.shortcut;
                else if (e->entry->type == OB_MENU_ENTRY_TYPE_SUBMENU)
                    entrykey = e->entry->data.submenu.submenu->shortcut;

                if (unikey == entrykey) {
                    if (found == NULL) found = e;
                    ++num_found;
                }

                /* next with wraparound */
                it = g_list_next(it);
                if (it == NULL) it = frame->entries;
            } while (it != start);

            if (found) {
                if (found->entry->type == OB_MENU_ENTRY_TYPE_NORMAL &&
                    num_found == 1)
                {
                    menu_frame_select(frame, found, TRUE);
                    usleep(50000); /* highlight the item for a short bit so the
                                      user can see what happened */
                    menu_entry_frame_execute(found, state);
                } else {
                    menu_frame_select(frame, found, TRUE);
                    if (num_found == 1)
                        menu_frame_select_next(frame->child);
                }

                ret = TRUE;
            }
        }
    }

    return ret;
}

static Bool event_look_for_menu_enter(Display *d, XEvent *ev, XPointer arg)
{
    ObMenuFrame *f = (ObMenuFrame*)arg;
    ObMenuEntryFrame *e;
    return ev->type == EnterNotify &&
        (e = g_hash_table_lookup(menu_frame_map, &ev->xcrossing.window)) &&
        !e->ignore_enters && e->frame == f;
}

static gboolean event_handle_menu(XEvent *ev)
{
    ObMenuFrame *f;
    ObMenuEntryFrame *e;
    gboolean ret = TRUE;

    switch (ev->type) {
    case ButtonRelease:
        if (menu_hide_delay_reached() &&
            (ev->xbutton.button < 4 || ev->xbutton.button > 5))
        {
            if ((e = menu_entry_frame_under(ev->xbutton.x_root,
                                            ev->xbutton.y_root)))
            {
                menu_frame_select(e->frame, e, TRUE);
                menu_entry_frame_execute(e, ev->xbutton.state);
            }
            else
                menu_frame_hide_all();
        }
        break;
    case EnterNotify:
        if ((e = g_hash_table_lookup(menu_frame_map, &ev->xcrossing.window))) {
            if (e->ignore_enters)
                --e->ignore_enters;
            else if (!(f = find_active_menu()) ||
                     f == e->frame ||
                     f->parent == e->frame ||
                     f->child == e->frame)
                menu_frame_select(e->frame, e, FALSE);
        }
        break;
    case LeaveNotify:
        /*ignore leaves when we're already in the window */
        if (ev->xcrossing.detail == NotifyInferior)
            break;

        if ((e = g_hash_table_lookup(menu_frame_map, &ev->xcrossing.window)))
        {
            XEvent ce;

            /* check if an EnterNotify event is coming, and if not, then select
               nothing in the menu */
            if (XCheckIfEvent(ob_display, &ce, event_look_for_menu_enter,
                              (XPointer)e->frame))
                XPutBackEvent(ob_display, &ce);
            else
                menu_frame_select(e->frame, NULL, FALSE);
        }
        break;
    case MotionNotify:
        if ((e = menu_entry_frame_under(ev->xmotion.x_root,
                                        ev->xmotion.y_root)))
            if (!(f = find_active_menu()) ||
                f == e->frame ||
                f->parent == e->frame ||
                f->child == e->frame)
                menu_frame_select(e->frame, e, FALSE);
        break;
    case KeyPress:
    case KeyRelease:
        ret = event_handle_menu_keyboard(ev);
        break;
    }
    return ret;
}

static void event_handle_user_input(ObClient *client, XEvent *e)
{
    g_assert(e->type == ButtonPress || e->type == ButtonRelease ||
             e->type == MotionNotify || e->type == KeyPress ||
             e->type == KeyRelease);

    if (menu_frame_visible) {
        if (event_handle_menu(e))
            /* don't use the event if the menu used it, but if the menu
               didn't use it and it's a keypress that is bound, it will
               close the menu and be used */
            return;
    }

    /* if the keyboard interactive action uses the event then dont
       use it for bindings. likewise is moveresize uses the event. */
    if (!actions_interactive_input_event(e) && !moveresize_event(e)) {
        if (moveresize_in_progress)
            /* make further actions work on the client being
               moved/resized */
            client = moveresize_client;

        if (e->type == ButtonPress ||
            e->type == ButtonRelease ||
            e->type == MotionNotify)
        {
            /* the frame may not be "visible" but they can still click on it
               in the case where it is animating before disappearing */
            if (!client || !frame_iconify_animating(client->frame))
                mouse_event(client, e);
        } else
            keyboard_event((focus_cycle_target ? focus_cycle_target :
                            (client ? client : focus_client)), e);
    }
}

static void focus_delay_dest(gpointer data)
{
    g_free(data);
}

static gboolean focus_delay_cmp(gconstpointer d1, gconstpointer d2)
{
    const ObFocusDelayData *f1 = d1;
    return f1->client == d2;
}

static gboolean focus_delay_func(gpointer data)
{
    ObFocusDelayData *d = data;
    Time old = event_curtime;

    event_curtime = d->time;
    event_curserial = d->serial;
    if (client_focus(d->client) && config_focus_raise)
        stacking_raise(CLIENT_AS_WINDOW(d->client));
    event_curtime = old;
    return FALSE; /* no repeat */
}

static void focus_delay_client_dest(ObClient *client, gpointer data)
{
    ob_main_loop_timeout_remove_data(ob_main_loop, focus_delay_func,
                                     client, FALSE);
}

void event_halt_focus_delay(void)
{
    /* ignore all enter events up till the event which caused this to occur */
    if (event_curserial) event_ignore_enter_range(1, event_curserial);
    ob_main_loop_timeout_remove(ob_main_loop, focus_delay_func);
}

gulong event_start_ignore_all_enters(void)
{
    return NextRequest(ob_display);
}

static void event_ignore_enter_range(gulong start, gulong end)
{
    ObSerialRange *r;

    g_assert(start != 0);
    g_assert(end != 0);

    r = g_new(ObSerialRange, 1);
    r->start = start;
    r->end = end;
    ignore_serials = g_slist_prepend(ignore_serials, r);

    ob_debug_type(OB_DEBUG_FOCUS, "ignoring enters from %lu until %lu\n",
                  r->start, r->end);

    /* increment the serial so we don't ignore events we weren't meant to */
    PROP_ERASE(screen_support_win, motif_wm_hints);
}

void event_end_ignore_all_enters(gulong start)
{
    /* Use (NextRequest-1) so that we ignore up to the current serial only.
       Inside event_ignore_enter_range, we increment the serial by one, but if
       we ignore that serial too, then any enter events generated by mouse
       movement will be ignored until we create some further network traffic.
       Instead ignore up to NextRequest-1, then when we increment the serial,
       we will be *past* the range of ignored serials */
    event_ignore_enter_range(start, NextRequest(ob_display)-1);
}

static gboolean is_enter_focus_event_ignored(gulong serial)
{
    GSList *it, *next;

    for (it = ignore_serials; it; it = next) {
        ObSerialRange *r = it->data;

        next = g_slist_next(it);

        if ((glong)(serial - r->end) > 0) {
            /* past the end */
            ignore_serials = g_slist_delete_link(ignore_serials, it);
            g_free(r);
        }
        else if ((glong)(serial - r->start) >= 0)
            return TRUE;
    }
    return FALSE;
}

void event_cancel_all_key_grabs(void)
{
    if (actions_interactive_act_running()) {
        actions_interactive_cancel_act();
        ob_debug("KILLED interactive action\n");
    }
    else if (menu_frame_visible) {
        menu_frame_hide_all();
        ob_debug("KILLED open menus\n");
    }
    else if (moveresize_in_progress) {
        moveresize_end(TRUE);
        ob_debug("KILLED interactive moveresize\n");
    }
    else if (grab_on_keyboard()) {
        ungrab_keyboard();
        ob_debug("KILLED active grab on keyboard\n");
    }
    else
        ungrab_passive_key();

    XSync(ob_display, FALSE);
}

gboolean event_time_after(guint32 t1, guint32 t2)
{
    g_assert(t1 != CurrentTime);
    g_assert(t2 != CurrentTime);

    /*
      Timestamp values wrap around (after about 49.7 days). The server, given
      its current time is represented by timestamp T, always interprets
      timestamps from clients by treating half of the timestamp space as being
      later in time than T.
      - http://tronche.com/gui/x/xlib/input/pointer-grabbing.html
    */

    /* TIME_HALF is not half of the number space of a Time type variable.
     * Rather, it is half the number space of a timestamp value, which is
     * always 32 bits. */
#define TIME_HALF (guint32)(1 << 31)

    if (t2 >= TIME_HALF)
        /* t2 is in the second half so t1 might wrap around and be smaller than
           t2 */
        return t1 >= t2 || t1 < (t2 + TIME_HALF);
    else
        /* t2 is in the first half so t1 has to come after it */
        return t1 >= t2 && t1 < (t2 + TIME_HALF);
}

Time event_get_server_time(void)
{
    /* Generate a timestamp */
    XEvent event;

    XChangeProperty(ob_display, screen_support_win,
                    prop_atoms.wm_class, prop_atoms.string,
                    8, PropModeAppend, NULL, 0);
    XWindowEvent(ob_display, screen_support_win, PropertyChangeMask, &event);
    return event.xproperty.time;
}
